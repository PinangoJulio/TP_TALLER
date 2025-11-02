#include "lobby_protocol.h"
#include <cstring>
#include <netinet/in.h>

namespace LobbyProtocol {

// Serializar username
std::vector<uint8_t> serialize_username(const std::string& username) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_USERNAME);
    
    uint16_t len = username.length();
    uint16_t len_net = htons(len);
    
    buffer.push_back((len_net >> 8) & 0xFF);
    buffer.push_back(len_net & 0xFF);
    
    for (char c : username) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    
    return buffer;
}

// Serializar petición de lista de juegos
std::vector<uint8_t> serialize_list_games() {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_LIST_GAMES);
    return buffer;
}

// Serializar petición de crear juego
std::vector<uint8_t> serialize_create_game(const std::string& game_name, uint8_t max_players) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_CREATE_GAME);
    
    uint16_t len = game_name.length();
    uint16_t len_net = htons(len);
    
    buffer.push_back((len_net >> 8) & 0xFF);
    buffer.push_back(len_net & 0xFF);
    
    for (char c : game_name) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    
    buffer.push_back(max_players);
    
    return buffer;
}

// Serializar petición de unirse a juego
std::vector<uint8_t> serialize_join_game(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_JOIN_GAME);
    
    uint16_t id_net = htons(game_id);
    buffer.push_back((id_net >> 8) & 0xFF);
    buffer.push_back(id_net & 0xFF);
    
    return buffer;
}

// Serializar mensaje de bienvenida
std::vector<uint8_t> serialize_welcome(const std::string& message) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_WELCOME);
    
    uint16_t len = message.length();
    uint16_t len_net = htons(len);
    
    buffer.push_back((len_net >> 8) & 0xFF);
    buffer.push_back(len_net & 0xFF);
    
    for (char c : message) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    
    return buffer;
}

// Serializar lista de juegos
std::vector<uint8_t> serialize_games_list(const std::vector<GameInfo>& games) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_GAMES_LIST);
    
    uint16_t count = games.size();
    uint16_t count_net = htons(count);
    
    buffer.push_back((count_net >> 8) & 0xFF);
    buffer.push_back(count_net & 0xFF);
    
    for (const auto& game : games) {
        uint16_t id_net = htons(game.game_id);
        buffer.push_back((id_net >> 8) & 0xFF);
        buffer.push_back(id_net & 0xFF);
        
        for (size_t i = 0; i < sizeof(game.game_name); ++i) {
            buffer.push_back(static_cast<uint8_t>(game.game_name[i]));
        }
        
        buffer.push_back(game.current_players);
        buffer.push_back(game.max_players);
        buffer.push_back(game.is_started ? 1 : 0);
    }
    
    return buffer;
}

// Serializar confirmación de juego creado
std::vector<uint8_t> serialize_game_created(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_GAME_CREATED);
    
    uint16_t id_net = htons(game_id);
    buffer.push_back((id_net >> 8) & 0xFF);
    buffer.push_back(id_net & 0xFF);
    
    return buffer;
}

// Serializar confirmación de unión a juego
std::vector<uint8_t> serialize_game_joined(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_GAME_JOINED);
    
    uint16_t id_net = htons(game_id);
    buffer.push_back((id_net >> 8) & 0xFF);
    buffer.push_back(id_net & 0xFF);
    
    return buffer;
}

// Serializar error
std::vector<uint8_t> serialize_error(LobbyErrorCode error_code, const std::string& message) {
    std::vector<uint8_t> buffer;
    
    buffer.push_back(MSG_ERROR);
    buffer.push_back(static_cast<uint8_t>(error_code));
    
    uint16_t len = message.length();
    uint16_t len_net = htons(len);
    
    buffer.push_back((len_net >> 8) & 0xFF);
    buffer.push_back(len_net & 0xFF);
    
    for (char c : message) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    
    return buffer;
}

} // namespace LobbyProtocol
