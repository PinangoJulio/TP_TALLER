#include "lobby_protocol.h"
#include <cstring>
#include <netinet/in.h>

namespace LobbyProtocol {

// Helper: Agregar uint16_t en big-endian
static void push_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back((value >> 8) & 0xFF);  // Byte alto
    buffer.push_back(value & 0xFF);         // Byte bajo
}

// Serializar username
std::vector<uint8_t> serialize_username(const std::string& username) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_USERNAME);
    push_uint16(buffer, username.length());
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
    push_uint16(buffer, game_name.length());
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
    push_uint16(buffer, game_id);
    return buffer;
}

// Serializar mensaje de bienvenida
std::vector<uint8_t> serialize_welcome(const std::string& message) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_WELCOME);
    push_uint16(buffer, message.length());
    for (char c : message) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    return buffer;
}

// Serializar lista de juegos
std::vector<uint8_t> serialize_games_list(const std::vector<GameInfo>& games) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAMES_LIST);
    push_uint16(buffer, games.size());
    
    for (const auto& game : games) {
        push_uint16(buffer, game.game_id);
        
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
    push_uint16(buffer, game_id);
    return buffer;
}

// Serializar confirmación de unión a juego
std::vector<uint8_t> serialize_game_joined(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAME_JOINED);
    push_uint16(buffer, game_id);
    return buffer;
}

// Serializar error
std::vector<uint8_t> serialize_error(LobbyErrorCode error_code, const std::string& message) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_ERROR);
    buffer.push_back(static_cast<uint8_t>(error_code));
    push_uint16(buffer, message.length());
    for (char c : message) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    return buffer;
}

std::vector<uint8_t> serialize_game_started(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_GAME_STARTED);
    push_uint16(buffer, game_id);
    return buffer;
}

std::vector<uint8_t> serialize_select_car(uint8_t car_index) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_SELECT_CAR);
    buffer.push_back(car_index);
    return buffer;
}

std::vector<uint8_t> serialize_start_game(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_START_GAME);
    push_uint16(buffer, game_id);
    return buffer;
}

std::vector<uint8_t> serialize_leave_game(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_LEAVE_GAME);
    push_uint16(buffer, game_id);
    return buffer;
}

} // namespace LobbyProtocol
