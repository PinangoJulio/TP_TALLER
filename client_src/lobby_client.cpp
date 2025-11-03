#include "lobby_client.h"
#include <iostream>
#include <netinet/in.h>

LobbyClient::LobbyClient(const std::string& host, const std::string& port)
    : socket(host.c_str(), port.c_str()), connected(true) {
    std::cout << "[LobbyClient] Connected to server " << host << ":" << port << std::endl;
}

uint8_t LobbyClient::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        connected = false;
        throw std::runtime_error("Connection closed by server");
    }
    return type;
}

std::string LobbyClient::read_string() {
    uint16_t len_net;
    socket.recvall(&len_net, sizeof(len_net));
    uint16_t len = ntohs(len_net);
    
    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    
    return std::string(buffer.begin(), buffer.end());
}

uint16_t LobbyClient::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void LobbyClient::send_username(const std::string& username) {
    this->username = username;
    auto buffer = LobbyProtocol::serialize_username(username);
    
    // DEBUG: Imprimir el buffer
    std::cout << "[LobbyClient] DEBUG: Username: '" << username 
              << "' (length: " << username.length() << ")" << std::endl;
    std::cout << "[LobbyClient] DEBUG: Buffer size: " << buffer.size() << " bytes" << std::endl;
    std::cout << "[LobbyClient] DEBUG: Buffer content: ";
    for (size_t i = 0; i < buffer.size(); ++i) {
        printf("%02X ", buffer[i]);
    }
    std::cout << std::endl;
    
    // Decodificar manualmente para verificar
    if (buffer.size() >= 3) {
        uint16_t len_sent = (buffer[1] << 8) | buffer[2];
        std::cout << "[LobbyClient] DEBUG: Length encoded in buffer: " << len_sent << std::endl;
    }
    
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Sent username: " << username << std::endl;
}

std::string LobbyClient::receive_welcome() {
    std::cout << "[LobbyClient] DEBUG: Waiting for welcome message..." << std::endl;
    uint8_t type = read_message_type();
    std::cout << "[LobbyClient] DEBUG: Received type: " << static_cast<int>(type) << std::endl;
    
    if (type != MSG_WELCOME) {
        throw std::runtime_error("Expected WELCOME message");
    }
    
    std::cout << "[LobbyClient] DEBUG: Reading welcome string..." << std::endl;
    std::string message = read_string();
    std::cout << "[LobbyClient] Received welcome: " << message << std::endl;
    return message;
}

void LobbyClient::request_games_list() {
    auto buffer = LobbyProtocol::serialize_list_games();
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Requested games list" << std::endl;
}

std::vector<GameInfo> LobbyClient::receive_games_list() {
    uint8_t type = read_message_type();
    if (type != MSG_GAMES_LIST) {
        throw std::runtime_error("Expected GAMES_LIST message");
    }
    
    uint16_t count = read_uint16();
    std::vector<GameInfo> games;
    
    for (uint16_t i = 0; i < count; ++i) {
        GameInfo info;
        info.game_id = read_uint16();
        socket.recvall(info.game_name, sizeof(info.game_name));
        socket.recvall(&info.current_players, sizeof(info.current_players));
        socket.recvall(&info.max_players, sizeof(info.max_players));
        
        uint8_t started;
        socket.recvall(&started, sizeof(started));
        info.is_started = (started != 0);
        
        games.push_back(info);
    }
    
    std::cout << "[LobbyClient] Received " << games.size() << " games" << std::endl;
    return games;
}

void LobbyClient::create_game(const std::string& game_name, uint8_t max_players) {
    auto buffer = LobbyProtocol::serialize_create_game(game_name, max_players);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Requested to create game: " << game_name << std::endl;
}

uint16_t LobbyClient::receive_game_created() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_CREATED) {
        throw std::runtime_error("Expected GAME_CREATED message");
    }
    
    uint16_t game_id = read_uint16();
    std::cout << "[LobbyClient] Game created with ID: " << game_id << std::endl;
    return game_id;
}

void LobbyClient::join_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_join_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Requested to join game: " << game_id << std::endl;
}

uint16_t LobbyClient::receive_game_joined() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_JOINED) {
        throw std::runtime_error("Expected GAME_JOINED message");
    }
    
    uint16_t game_id = read_uint16();
    std::cout << "[LobbyClient] Joined game: " << game_id << std::endl;
    return game_id;
}

uint8_t LobbyClient::peek_message_type() {
    return read_message_type();
}

void LobbyClient::read_error_details(std::string& error_message) {
    // Leer código de error
    uint8_t error_code;
    socket.recvall(&error_code, sizeof(error_code));
    
    // Leer mensaje de error
    error_message = read_string();
    
    std::cout << "[LobbyClient] Error received (code " << static_cast<int>(error_code) 
              << "): " << error_message << std::endl;
}