#include "client_protocol.h"
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include "common_src/lobby_protocol.h"

ClientProtocol::ClientProtocol(const char* host, const char* servname)
    : socket(Socket(host, servname)), host(host), port(servname) {
    std::cout << "[ClientProtocol] Connected to server " << host << ":" << servname << std::endl;
}
ClientProtocol::~ClientProtocol() { if (!socket_shutdown_done) try { shutdown_socket(); } catch (...) {} }
void ClientProtocol::shutdown_socket() { if (!socket_shutdown_done) { socket.shutdown(2); socket_shutdown_done=true; std::cout << "[ClientProtocol] Socket shutdown completed" << std::endl; } }
void ClientProtocol::disconnect() { shutdown_socket(); std::cout << "[ClientProtocol] Disconnected from server" << std::endl; }

uint8_t ClientProtocol::read_message_type() { uint8_t t; if (socket.recvall(&t, 1)==0) throw std::runtime_error("Connection closed by server"); return t; }
uint16_t ClientProtocol::read_uint16() { uint16_t v; socket.recvall(&v, 2); return ntohs(v); }
uint8_t ClientProtocol::read_uint8() { uint8_t v; socket.recvall(&v, 1); return v; }
uint32_t ClientProtocol::read_uint32() { uint32_t v; socket.recvall(&v, 4); return ntohl(v); }
int16_t ClientProtocol::read_int16() { uint16_t v; socket.recvall(&v, 2); return (int16_t)ntohs(v); }
int32_t ClientProtocol::read_int32() { uint32_t v; socket.recvall(&v, 4); return (int32_t)ntohl(v); }

std::string ClientProtocol::read_string() {
    uint16_t len = read_uint16();
    std::vector<char> buf(len);
    socket.recvall(buf.data(), len);
    return std::string(buf.begin(), buf.end());
}

void ClientProtocol::send_string(const std::string& str) {
    uint16_t len = htons(str.size());
    socket.sendall(&len, 2);
    socket.sendall(str.data(), str.size());
}

// --------------------------------------------------------------------------
// LOBBY
// --------------------------------------------------------------------------
void ClientProtocol::send_username(const std::string& user) {
    auto buffer = LobbyProtocol::serialize_username(user);
    std::cout << "[Protocol] DEBUG: Username: '" << user << "'\n";
    // (Logs de tus compañeros...)
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Sent username: " << user << std::endl;
}

void ClientProtocol::request_games_list() {
    auto buffer = LobbyProtocol::serialize_list_games();
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Requested games list" << std::endl;
}

std::vector<GameInfo> ClientProtocol::read_games_list_from_socket(uint16_t count) {
    std::vector<GameInfo> games;
    for (uint16_t i = 0; i < count; i++) {
        GameInfo info;
        info.game_id = read_uint16();
        socket.recvall(info.game_name, sizeof(info.game_name));
        socket.recvall(&info.current_players, 1);
        socket.recvall(&info.max_players, 1);
        uint8_t st; socket.recvall(&st, 1); info.is_started = st;
        games.push_back(info);
        std::cout << "[Protocol]    Game " << info.game_id << ": " << info.game_name << std::endl;
    }
    return games;
}

std::vector<GameInfo> ClientProtocol::receive_games_list() {
    if (read_message_type() != MSG_GAMES_LIST) throw std::runtime_error("Expected GAMES_LIST");
    uint16_t count = read_uint16();
    auto games = read_games_list_from_socket(count);
    std::cout << "[Protocol] Received " << games.size() << " games" << std::endl;
    return games;
}

void ClientProtocol::create_game(const std::string& n, uint8_t m, const std::vector<std::pair<std::string, std::string>>& r) {
    auto b = LobbyProtocol::serialize_create_game(n, m, r.size());
    socket.sendall(b.data(), b.size());
    for(auto& x : r) { send_string(x.first); send_string(x.second); }
    std::cout << "[Protocol] Requested create game: " << n << "\n";
}

uint16_t ClientProtocol::receive_game_created() {
    if (read_message_type() != MSG_GAME_CREATED) throw std::runtime_error("Exp GAME_CREATED");
    uint16_t id = read_uint16();
    std::cout << "[Protocol] Game created ID: " << id << "\n";
    return id;
}

void ClientProtocol::join_game(uint16_t id) {
    auto b = LobbyProtocol::serialize_join_game(id);
    socket.sendall(b.data(), b.size());
    std::cout << "[Protocol] Req join game: " << id << "\n";
}

uint16_t ClientProtocol::receive_game_joined() {
    uint8_t t = read_message_type();
    if(t == MSG_ERROR) { std::string e; read_error_details(e); throw std::runtime_error(e); }
    if(t != MSG_GAME_JOINED) throw std::runtime_error("Exp GAME_JOINED");
    uint16_t id = read_uint16();
    std::cout << "[Protocol] Joined game: " << id << "\n";
    return id;
}

void ClientProtocol::read_error_details(std::string& e) {
    uint8_t c; socket.recvall(&c,1); e = read_string();
    std::cout << "[Protocol] Error (code " << (int)c << "): " << e << "\n";
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> ClientProtocol::receive_city_maps() {
    if (read_message_type() != MSG_CITY_MAPS) throw std::runtime_error("Exp CITY_MAPS");
    uint16_t nc = read_uint16();
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> res;
    for(int i=0; i<nc; ++i) {
        std::string city = read_string();
        uint16_t nm = read_uint16();
        std::vector<std::pair<std::string, std::string>> maps;
        for(int j=0; j<nm; ++j) {
            std::string y = read_string();
            std::string p = read_string();
            maps.emplace_back(y,p);
        }
        res.emplace_back(city, maps);
    }
    return res;
}

void ClientProtocol::send_selected_races(const std::vector<std::pair<std::string, std::string>>& races) {
    for (const auto& [city, map] : races) {
        auto b_city = LobbyProtocol::serialize_string(city);
        auto b_map = LobbyProtocol::serialize_string(map);
        socket.sendall(b_city.data(), b_city.size());
        socket.sendall(b_map.data(), b_map.size());
        std::cout << "[Protocol] Selected race: " << city << " - " << map << std::endl;
    }
}

void ClientProtocol::select_car(const std::string& car_name, const std::string& car_type) {
    auto buffer = LobbyProtocol::serialize_select_car(car_name, car_type);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Selected car: " << car_name << "\n";
}

void ClientProtocol::start_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_game_started(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Start request sent ID: " << game_id << "\n";
}

void ClientProtocol::leave_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_leave_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Leave request sent ID: " << game_id << "\n";
}

void ClientProtocol::set_ready(bool is_ready) {
    auto buffer = LobbyProtocol::serialize_player_ready(is_ready);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Set ready: " << (is_ready ? "YES" : "NO") << std::endl;
}

// --------------------------------------------------------------------------
// GAME COMMANDS
// --------------------------------------------------------------------------
void ClientProtocol::send_command_client(const ComandMatchDTO& c) {
    std::vector<uint8_t> msg;
    msg.push_back((uint8_t)c.command);
    switch (c.command) {
        case GameCommand::TURN_LEFT:
        case GameCommand::TURN_RIGHT:
            msg.push_back((uint8_t)(c.turn_intensity * 100.0f)); break;
        case GameCommand::UPGRADE_SPEED:
        case GameCommand::UPGRADE_ACCELERATION:
        case GameCommand::UPGRADE_HANDLING:
        case GameCommand::UPGRADE_DURABILITY:
            msg.push_back(c.upgrade_level);
            push_back_uint16(msg, c.upgrade_cost_ms); break;
        default: break;
    }
    if (!socket.sendall(msg.data(), msg.size())) throw std::runtime_error("Error sending cmd");
}

void ClientProtocol::serialize_command(const ComandMatchDTO&, std::vector<uint8_t>&) { /* Unused here */ }

void ClientProtocol::push_back_uint16(std::vector<uint8_t>& message, std::uint16_t value) {
    uint16_t net_value = htons(value);
    message.push_back(((uint8_t*)&net_value)[0]);
    message.push_back(((uint8_t*)&net_value)[1]);
}

// --------------------------------------------------------------------------
// RECEIVE SNAPSHOT 
// --------------------------------------------------------------------------
GameState ClientProtocol::receive_snapshot() {
    // ⚠️ NO LEEMOS EL TIPO.
    GameState state;

    uint16_t player_count = read_uint16();
    state.players.resize(player_count);

    for (uint16_t i = 0; i < player_count; ++i) {
        InfoPlayer& p = state.players[i];
        p.username  = read_string();
        p.car_name  = read_string();
        p.car_type  = read_string();
        p.player_id = read_uint16();
        p.pos_x = (float)read_int32() / 100.0f;
        p.pos_y = (float)read_int32() / 100.0f;
        p.angle = (float)read_uint16() / 100.0f;
        p.speed = (float)read_uint16() / 100.0f;
        p.velocity_x = (float)read_int32() / 100.0f;
        p.velocity_y = (float)read_int32() / 100.0f;
        p.health       = read_uint8();
        p.nitro_amount = read_uint8();
        uint8_t flags = read_uint8();
        p.nitro_active = (flags & 0x01);
        p.is_drifting  = (flags & 0x02);
        p.is_colliding = (flags & 0x04);
        p.completed_laps     = read_uint16();
        p.current_checkpoint = read_uint16();
        p.position_in_race   = read_uint8();
        p.race_time_ms       = read_uint32();
        p.race_finished = read_uint8();
        p.is_alive      = read_uint8();
        p.disconnected  = read_uint8();

        // 🏎️ PRINT PARA VER DATOS 🏎️
        std::cout << "[ClientProtocol]    🏎️  Player " << p.player_id
                  << ": pos=(" << p.pos_x << "," << p.pos_y << ") "
                  << "vel=" << p.speed << " km/h "
                  << "angle=" << p.angle << "° "
                  << "hp=" << (int)p.health
                  << (p.nitro_active ? " ⚡" : "") << std::endl;
    }

    uint16_t count = read_uint16();
    state.checkpoints.reserve(count);
    for(int i=0; i<count; ++i) {
        CheckpointInfo c;
        c.id = read_uint32();
        c.pos_x = (float)read_int32() / 100.0f;
        c.pos_y = (float)read_int32() / 100.0f;
        c.width = (float)read_uint16() / 100.0f;
        c.angle = (float)read_uint16() / 100.0f;
        c.is_start = read_uint8();
        c.is_finish = read_uint8();
        state.checkpoints.push_back(std::move(c));
    }

    count = read_uint16();
    state.npcs.reserve(count);
    for(int i=0; i<count; ++i) {
        NPCCarInfo n;
        n.npc_id = read_uint32();
        n.pos_x = (float)read_int32() / 100.0f;
        n.pos_y = (float)read_int32() / 100.0f;
        n.angle = (float)read_uint16() / 100.0f;
        n.speed = (float)read_uint16() / 100.0f;
        n.is_parked = read_uint8();
        state.npcs.push_back(std::move(n));
    }

    state.race_info.status = (MatchStatus)read_uint8();
    state.race_info.race_number = read_uint8();
    state.race_info.total_races = read_uint8();
    state.race_info.remaining_time_ms = read_uint32();
    state.race_info.players_finished = read_uint8();
    state.race_info.total_players = read_uint8();

    count = read_uint16();
    state.events.reserve(count);
    for(int i=0; i<count; ++i) {
        GameEvent e;
        e.type = (GameEvent::EventType)read_uint8();
        e.player_id = read_uint32();
        e.pos_x = (float)read_int32() / 100.0f;
        e.pos_y = (float)read_int32() / 100.0f;
        state.events.push_back(std::move(e));
    }
    return state;
}

int ClientProtocol::receive_client_id() {
    uint16_t id = read_uint16();
    std::cout << "[ClientProtocol] Received client ID: " << id << std::endl;
    return (int)id;
}

RaceInfoDTO ClientProtocol::receive_race_info() {
    if (read_message_type() != (uint8_t)ServerMessageType::RACE_INFO) throw std::runtime_error("Exp RACE_INFO");
    RaceInfoDTO r; std::memset(&r,0,sizeof(r));
    std::string s;
    s = read_string(); strncpy(r.city_name, s.c_str(), 19);
    s = read_string(); strncpy(r.race_name, s.c_str(), 19);
    s = read_string(); strncpy(r.map_file_path, s.c_str(), 49);
    r.total_laps = read_uint8();
    r.race_number = read_uint8();
    r.total_races = read_uint8();
    r.total_checkpoints = read_uint16();
    r.max_time_ms = read_uint32();
    
    std::cout << "[ClientProtocol] ✅ Race info received:\n"
              << "   City: " << r.city_name << "\n"
              << "   Race: " << r.race_name << "\n";
    return r;
}

std::vector<std::string> ClientProtocol::receive_race_paths() {
    if (read_message_type() != (uint8_t)ServerMessageType::RACE_PATHS) throw std::runtime_error("Exp RACE_PATHS");
    uint8_t nr = read_uint8();
    std::vector<std::string> p;
    for(int i=0; i<nr; ++i) {
        std::string s = read_string();
        p.push_back(s);
        std::cout << "[ClientProtocol]    Race " << (i+1) << ": " << s << "\n";
    }
    std::cout << "[ClientProtocol] ✅ Received " << (int)nr << " race paths\n";
    return p;
}