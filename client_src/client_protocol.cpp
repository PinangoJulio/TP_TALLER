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
    socket.sendall(buffer.data(), buffer.size());
}

void ClientProtocol::serialize_command(const ComandMatchDTO& command,
                                       std::vector<uint8_t>& message) {
    message.push_back(static_cast<uint8_t>(command.command));
    push_back_uint16(message, command.player_id);
}

void ClientProtocol::push_back_uint16(std::vector<uint8_t>& message, std::uint16_t value) {
    uint16_t net_value = htons(value);
    message.push_back(reinterpret_cast<uint8_t*>(&net_value)[0]);
    message.push_back(reinterpret_cast<uint8_t*>(&net_value)[1]);
}

GameState ClientProtocol::receive_snapshot() {
    // TODO: Implementar deserialización completa del snapshot
    // Por ahora, bloqueamos esperando un byte para evitar loop infinito
    try {
        uint8_t msg_type;
        socket.recvall(&msg_type, sizeof(msg_type));

        // TODO: Aquí debería ir la deserialización completa del GameState
        // Por ahora retornamos un snapshot vacío pero al menos bloqueamos
        std::cout << "[ClientProtocol] Received snapshot message type: "
                  << static_cast<int>(msg_type) << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[ClientProtocol] Error receiving snapshot: " << e.what() << std::endl;
        throw;
    }

    GameState snapshot;
    return snapshot;
}

int ClientProtocol::receive_client_id() {
    // Leer el ID del cliente enviado por el servidor
    uint16_t client_id = read_uint16();
    std::cout << "[ClientProtocol] Received client ID: " << client_id << std::endl;
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

    std::cout << "[ClientProtocol] ✅ Race info received:" << std::endl;
    std::cout << "[ClientProtocol]   City: " << race_info.city_name << std::endl;
    std::cout << "[ClientProtocol]   Race: " << race_info.race_name << std::endl;
    std::cout << "[ClientProtocol]   Map: " << race_info.map_file_path << std::endl;
    std::cout << "[ClientProtocol]   Lap " << static_cast<int>(race_info.race_number) << "/"
              << static_cast<int>(race_info.total_races) << std::endl;

    return race_info;
}
