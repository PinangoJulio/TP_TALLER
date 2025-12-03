#include "client_protocol.h"

#include <arpa/inet.h>  // ntohl
#include <netinet/in.h>

#include <cstring>  // memset, strncpy
#include <iostream>
#include <stdexcept>
#include <utility>

#include "common_src/lobby_protocol.h"

ClientProtocol::ClientProtocol(const char* host, const char* servname)
    : socket(Socket(host, servname)), host(host), port(servname) {
    std::cout << "[ClientProtocol] Connected to server " << host << ":" << servname << std::endl;
}

ClientProtocol::~ClientProtocol() {
    if (!socket_shutdown_done) {
        try {
            shutdown_socket();
        } catch (...) {
            // Ignorar errores en el destructor
        }
    }
}

void ClientProtocol::shutdown_socket() {
    if (!socket_shutdown_done) {
        try {
            socket.shutdown(2);  // SHUT_RDWR
            socket_shutdown_done = true;
            std::cout << "[ClientProtocol] Socket shutdown completed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[ClientProtocol] Error in shutdown: " << e.what() << std::endl;
        }
    }
}

void ClientProtocol::disconnect() {
    shutdown_socket();
    std::cout << "[ClientProtocol] Disconnected from server" << std::endl;
}

uint8_t ClientProtocol::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        throw std::runtime_error("Connection closed by server");
    }
    return type;
}

std::string ClientProtocol::read_string() {
    uint16_t len_net;
    socket.recvall(&len_net, sizeof(len_net));
    uint16_t len = ntohs(len_net);

    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);

    return std::string(buffer.begin(), buffer.end());
}

uint16_t ClientProtocol::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

uint8_t ClientProtocol::read_uint8() {
    uint8_t value;
    socket.recvall(&value, sizeof(value));
    return value;
}

void ClientProtocol::send_string(const std::string& str) {
    uint16_t len = htons(static_cast<uint16_t>(str.size()));
    socket.sendall(&len, sizeof(len));
    socket.sendall(str.data(), str.size());
}

// Lobby protocol methods

void ClientProtocol::send_username(const std::string& user) {
    auto buffer = LobbyProtocol::serialize_username(user);

    std::cout << "[Protocol] DEBUG: Username: '" << user << "' (length: " << user.length() << ")"
              << std::endl;
    std::cout << "[Protocol] DEBUG: Buffer size: " << buffer.size() << " bytes" << std::endl;
    std::cout << "[Protocol] DEBUG: Buffer content: ";
    for (size_t i = 0; i < buffer.size(); ++i) {
        printf("%02X ", buffer[i]);
    }
    std::cout << std::endl;

    if (buffer.size() >= 3) {
        uint16_t len_sent = (buffer[1] << 8) | buffer[2];
        std::cout << "[Protocol] DEBUG: Length encoded in buffer: " << len_sent << std::endl;
    }

    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Sent username: " << user << std::endl;
}

void ClientProtocol::request_games_list() {
    auto buffer = LobbyProtocol::serialize_list_games();
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Requested games list" << std::endl;
}

// Helper para leer lista de juegos del socket
std::vector<GameInfo> ClientProtocol::read_games_list_from_socket(uint16_t count) {
    std::vector<GameInfo> games;

    for (uint16_t i = 0; i < count; i++) {
        GameInfo info;
        info.game_id = read_uint16();
        socket.recvall(info.game_name, sizeof(info.game_name));
        socket.recvall(&info.current_players, sizeof(info.current_players));
        socket.recvall(&info.max_players, sizeof(info.max_players));
        uint8_t started;
        socket.recvall(&started, sizeof(started));
        info.is_started = (started != 0);

        games.push_back(info);
        std::cout << "[Protocol]   Game " << info.game_id << ": " << info.game_name << std::endl;
    }

    return games;
}

std::vector<GameInfo> ClientProtocol::receive_games_list() {
    uint8_t type = read_message_type();
    if (type != MSG_GAMES_LIST) {
        throw std::runtime_error("Expected GAMES_LIST message");
    }

    uint16_t count = read_uint16();
    std::vector<GameInfo> games = read_games_list_from_socket(count);

    std::cout << "[Protocol] Received " << games.size() << " games" << std::endl;
    return games;
}

void ClientProtocol::create_game(const std::string& game_name, uint8_t max_players,
                                 const std::vector<std::pair<std::string, std::string>>& races) {
    auto buffer = LobbyProtocol::serialize_create_game(game_name, max_players, races.size());
    socket.sendall(buffer.data(), buffer.size());
    for (const auto& race : races) {
        send_string(race.first);  // city
        send_string(race.second);
    }
    std::cout << "[Protocol] Requested to create game: " << game_name
              << " (max players: " << static_cast<int>(max_players)
              << ", races: " << static_cast<int>(races.size()) << ")\n";
}

uint16_t ClientProtocol::receive_game_created() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_CREATED) {
        throw std::runtime_error("Expected GAME_CREATED message");
    }

    uint16_t game_id = read_uint16();
    std::cout << "[Protocol] Game created with ID: " << game_id << std::endl;
    return game_id;
}

void ClientProtocol::join_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_join_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Requested to join game: " << game_id << std::endl;
}

uint16_t ClientProtocol::receive_game_joined() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_JOINED) {
        throw std::runtime_error("Expected GAME_JOINED message");
    }

    uint16_t game_id = read_uint16();
    std::cout << "[Protocol] Joined game: " << game_id << std::endl;

    // El snapshot llegará vía notificaciones automáticamente

    return game_id;
}

void ClientProtocol::read_error_details(std::string& error_message) {
    uint8_t error_code;
    socket.recvall(&error_code, sizeof(error_code));

    error_message = read_string();

    std::cout << "[Protocol] Error received (code " << static_cast<int>(error_code)
              << "): " << error_message << std::endl;
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
ClientProtocol::receive_city_maps() {
    uint8_t type = read_message_type();
    if (type != MSG_CITY_MAPS) {
        throw std::runtime_error("Expected CITY_MAPS message");
    }

    uint16_t num_cities = read_uint16();
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;

    for (int i = 0; i < num_cities; ++i) {
        std::string city_name = read_string();
        uint16_t num_maps = read_uint16();

        std::vector<std::pair<std::string, std::string>> maps;
        for (int j = 0; j < num_maps; ++j) {
            std::string yaml = read_string();
            std::string png = read_string();
            maps.emplace_back(yaml, png);
        }

        result.emplace_back(city_name, maps);
    }

    return result;
}

void ClientProtocol::send_selected_races(
    const std::vector<std::pair<std::string, std::string>>& races) {
    for (const auto& [city, map] : races) {
        auto buffer_city = LobbyProtocol::serialize_string(city);
        auto buffer_map = LobbyProtocol::serialize_string(map);
        socket.sendall(buffer_city.data(), buffer_city.size());
        socket.sendall(buffer_map.data(), buffer_map.size());
        std::cout << "[Protocol] Selected race: " << city << " - " << map << std::endl;
    }
}

void ClientProtocol::select_car(const std::string& car_name, const std::string& car_type) {
    auto buffer = LobbyProtocol::serialize_select_car(car_name, car_type);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Selected car: " << car_name << " (" << car_type << ")\n";
}

void ClientProtocol::start_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_game_started(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Start game request sent for ID: " << game_id << std::endl;
}

void ClientProtocol::leave_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_leave_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Leave game request sent for ID: " << game_id << std::endl;
}

void ClientProtocol::set_ready(bool is_ready) {
    auto buffer = LobbyProtocol::serialize_player_ready(is_ready);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[Protocol] Set ready: " << (is_ready ? "YES" : "NO") << std::endl;
}

// GAME - Commands & Snapshots

void ClientProtocol::send_command_client(const ComandMatchDTO& command) {
    std::vector<uint8_t> buffer;
    serialize_command(command, buffer);
    if (!socket.sendall(buffer.data(), buffer.size())) {
        throw std::runtime_error("Error sending command");
    }}

void ClientProtocol::serialize_command(const ComandMatchDTO& command,
                                       std::vector<uint8_t>& message) {
    message.push_back(static_cast<uint8_t>(command.command));

    // Agregar datos adicionales según el comando
    switch (command.command) {
        case GameCommand::ACCELERATE:
        case GameCommand::BRAKE:
        case GameCommand::MOVE_UP:      // Nuevos comandos de movimiento
        case GameCommand::MOVE_DOWN:
        case GameCommand::MOVE_LEFT:
        case GameCommand::MOVE_RIGHT:
        case GameCommand::USE_NITRO:
        case GameCommand::STOP_ALL:
        case GameCommand::DISCONNECT:
        case GameCommand::CHEAT_INVINCIBLE:
        case GameCommand::CHEAT_WIN_RACE:
        case GameCommand::CHEAT_LOSE_RACE:
        case GameCommand::CHEAT_MAX_SPEED:
            // No requieren datos adicionales
            break;

        case GameCommand::TURN_LEFT:
        case GameCommand::TURN_RIGHT:
            // Agregar intensity (uint8_t, 0-100)
            message.push_back(static_cast<uint8_t>(command.turn_intensity * 100.0f));
            break;

        case GameCommand::UPGRADE_SPEED:
        case GameCommand::UPGRADE_ACCELERATION:
        case GameCommand::UPGRADE_HANDLING:
        case GameCommand::UPGRADE_DURABILITY:
            // Agregar level (uint8_t) y cost (uint16_t)
            message.push_back(command.upgrade_level);
            push_back_uint16(message, command.upgrade_cost_ms);
            break;

        default:
            // Comando desconocido: no agregar nada extra
            break;
    }
}

void ClientProtocol::push_back_uint16(std::vector<uint8_t>& message, std::uint16_t value) {
    uint16_t net_value = htons(value);
    message.push_back(reinterpret_cast<uint8_t*>(&net_value)[0]);
    message.push_back(reinterpret_cast<uint8_t*>(&net_value)[1]);
}

uint32_t ClientProtocol::read_uint32() {
    uint32_t value_net;
    socket.recvall(&value_net, sizeof(value_net));  // lee 4 bytes del socket (big endian)
    return ntohl(value_net); // convierte a host endian
}

int16_t ClientProtocol::read_int16() {
    uint16_t raw;
    socket.recvall(&raw, sizeof(raw));
    return (int16_t) ntohs(raw);   // Convierte preservando el signo
}

int32_t ClientProtocol::read_int32() {
    uint32_t net;
    socket.recvall(&net, sizeof(net));
    net = ntohl(net);
    return static_cast<int32_t>(net);
}

// (tu función está perfecta, no hace falta tocarla)
GameState ClientProtocol::receive_snapshot() {
    uint8_t type = read_message_type();
    if (type != (uint8_t)ServerMessageType::GAME_STATE_UPDATE) {

        // Manejo robusto: no intentes parsear como snapshot un mensaje diferente
        if (type == 0xFF /* MSG_ERROR */) {
            try {
                uint8_t err_code = read_uint8();
                // En nuestro servidor, para shutdown se envía: 0xFF, 0xFF, uint16 len, string
                std::string msg = "";
                try {
                    uint16_t len_net;
                    socket.recvall(&len_net, sizeof(len_net));
                    uint16_t len = ntohs(len_net);
                    if (len > 0 && len < 4096) {
                        std::vector<char> buf(len);
                        socket.recvall(buf.data(), len);
                        msg.assign(buf.begin(), buf.end());
                    }
                } catch (...) {
                    // Si el formato no coincide, ignoramos el texto
                }
                if (err_code == 0xFF) {
                    throw std::runtime_error(msg.empty() ? "Server shutdown" : msg);
                } else {
                    throw std::runtime_error(msg.empty() ? "Server error" : msg);
                }
            } catch (const std::exception& e) {
                throw; // Propaga para que el caller cierre ordenadamente
            }
        }

        // Tipo desconocido mientras estamos en juego: abortar para no desincronizar el stream
        throw std::runtime_error("Unexpected message type while expecting snapshot");
    }
    GameState state;

    // 1. PLAYERS
    uint16_t player_count = read_uint16();
    state.players.resize(player_count);

    for (uint16_t i = 0; i < player_count; ++i) {
        InfoPlayer& p = state.players[i];

        p.username  = read_string();
        p.car_name  = read_string();
        p.car_type  = read_string();

        p.player_id = read_uint16();

        p.pos_x = static_cast<float>(read_int32()) / 100.0f;
        p.pos_y = static_cast<float>(read_int32()) / 100.0f;

        p.angle = static_cast<float>(read_uint16()) / 100.0f;
        p.speed = static_cast<float>(read_uint16()) / 100.0f;

        p.velocity_x = static_cast<float>(read_int32()) / 100.0f;
        p.velocity_y = static_cast<float>(read_int32()) / 100.0f;

        p.health       = read_uint8();
        p.nitro_amount = read_uint8();

        uint8_t flags = read_uint8();
        p.nitro_active = (flags & 0x01) != 0;
        p.is_drifting  = (flags & 0x02) != 0;
        p.is_colliding = (flags & 0x04) != 0;

        p.completed_laps     = read_uint16();
        p.current_checkpoint = read_uint16();
        p.position_in_race   = read_uint8();

        p.race_time_ms = read_uint32();
        p.total_time_ms = read_uint32();

        p.race_finished = read_uint8() != 0;
        p.is_alive      = read_uint8() != 0;
        p.disconnected  = read_uint8() != 0;

    }

    uint16_t checkpointCount = read_uint16();
    state.checkpoints.clear();
    state.checkpoints.reserve(checkpointCount);

    for (uint16_t i = 0; i < checkpointCount; ++i) {
        CheckpointInfo c;
        c.id    = read_uint32();
        c.pos_x = static_cast<float>(read_int32()) / 100.0f;
        c.pos_y = static_cast<float>(read_int32()) / 100.0f;
        c.width = static_cast<float>(read_uint16()) / 100.0f;
        c.angle = static_cast<float>(read_uint16()) / 100.0f;
        c.is_start  = read_uint8() != 0;
        c.is_finish = read_uint8() != 0;
        state.checkpoints.push_back(std::move(c));
    }

    // 3. NPCs
    uint16_t npcCount = read_uint16();
    state.npcs.clear();
    state.npcs.reserve(npcCount);

    for (uint16_t i = 0; i < npcCount; ++i) {
        NPCCarInfo n;
        n.npc_id = read_uint32();
        n.pos_x = static_cast<float>(read_int32()) / 100.0f;
        n.pos_y = static_cast<float>(read_int32()) / 100.0f;
        n.angle = static_cast<float>(read_uint16()) / 100.0f;
        n.speed = static_cast<float>(read_uint16()) / 100.0f;
        n.is_parked = read_uint8() != 0;
        state.npcs.push_back(std::move(n));
    }

    // 4. RACE INFO
    state.race_info.status = static_cast<MatchStatus>(read_uint8());
    state.race_info.race_number       = read_uint8();
    state.race_info.total_races       = read_uint8();
    state.race_info.remaining_time_ms = read_uint32();
    state.race_info.players_finished  = read_uint8();
    state.race_info.total_players     = read_uint8();

    // 5. EVENTS
    uint16_t eventCount = read_uint16();
    state.events.clear();
    state.events.reserve(eventCount);

    for (uint16_t i = 0; i < eventCount; ++i) {
        GameEvent e;
        e.type      = (GameEvent::EventType)read_uint8();
        e.player_id = read_uint32();
        e.pos_x = static_cast<float>(read_int32()) / 100.0f;
        e.pos_y = static_cast<float>(read_int32()) / 100.0f;
        state.events.push_back(std::move(e));
    }

    return state;
}


int ClientProtocol::receive_client_id() {
    // Leer el ID del cliente enviado por el servidor
    uint16_t client_id = read_uint16();
    return static_cast<int>(client_id);
}

// ============================================================================
// RECEPCIÓN DE INFORMACIÓN DE CARRERA
// ============================================================================

RaceInfoDTO ClientProtocol::receive_race_info() {
    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));

    // 1. Leer tipo de mensaje (debe ser RACE_INFO)
    uint8_t msg_type = read_message_type();
    if (msg_type != static_cast<uint8_t>(ServerMessageType::RACE_INFO)) {
        throw std::runtime_error("Expected RACE_INFO message, got " + std::to_string(msg_type));
    }

    // 2. Leer ciudad
    std::string city = read_string();
    std::strncpy(race_info.city_name, city.c_str(), sizeof(race_info.city_name) - 1);

    // 3. Leer nombre de carrera
    std::string race = read_string();
    std::strncpy(race_info.race_name, race.c_str(), sizeof(race_info.race_name) - 1);

    // 4. Leer ruta del mapa
    std::string map_path = read_string();
    std::strncpy(race_info.map_file_path, map_path.c_str(), sizeof(race_info.map_file_path) - 1);

    // 5. Leer datos numéricos
    race_info.total_laps = read_uint8();
    race_info.race_number = read_uint8();
    race_info.total_races = read_uint8();
    race_info.total_checkpoints = read_uint16();

    uint32_t max_time_net;
    socket.recvall(&max_time_net, sizeof(max_time_net));
    race_info.max_time_ms = ntohl(max_time_net);


    return race_info;
}

// RECEPCIÓN DE RUTAS YAML DE CARRERAS

std::vector<std::string> ClientProtocol::receive_race_paths() {
    // Leer tipo de mensaje (debe ser RACE_PATHS)
    uint8_t msg_type = read_message_type();
    if (msg_type != static_cast<uint8_t>(ServerMessageType::RACE_PATHS)) {
        throw std::runtime_error("Expected RACE_PATHS message, got " + std::to_string(msg_type));
    }

    //Leer cantidad de carreras
    uint8_t num_races = read_uint8();

    std::vector<std::string> paths;
    paths.reserve(num_races);

    // Leer cada path
    for (uint8_t i = 0; i < num_races; ++i) {
        std::string path = read_string();
        paths.push_back(path);

    }


    return paths;
}
