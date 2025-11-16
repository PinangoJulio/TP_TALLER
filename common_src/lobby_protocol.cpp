#include "lobby_protocol.h"
#include <cstring>
#include <netinet/in.h>

namespace LobbyProtocol {

// Helper: Agregar uint16_t en big-endian
static void push_uint16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);         
}

static void push_string(std::vector<uint8_t>& buffer, const std::string& str) {
    push_uint16(buffer, str.size());
    buffer.insert(buffer.end(), str.begin(), str.end());
}

std::vector<uint8_t> serialize_player_joined_notification(const std::string& username) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_PLAYER_JOINED_NOTIFICATION);
    push_string(buffer, username);
    return buffer;
}

std::vector<uint8_t> serialize_player_left_notification(const std::string& username) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_PLAYER_LEFT_NOTIFICATION);
    push_string(buffer, username);
    return buffer;
}

std::vector<uint8_t> serialize_player_ready_notification(const std::string& username, bool is_ready) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_PLAYER_READY_NOTIFICATION);
    push_string(buffer, username);
    buffer.push_back(is_ready ? 1 : 0);
    return buffer;
}

std::vector<uint8_t> serialize_car_selected_notification(
    const std::string& username, 
    const std::string& car_name, 
    const std::string& car_type) 
{
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_CAR_SELECTED_NOTIFICATION);
    push_string(buffer, username);
    push_string(buffer, car_name);
    push_string(buffer, car_type);
    return buffer;
}

std::vector<uint8_t> serialize_player_ready(bool is_ready) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_PLAYER_READY);
    buffer.push_back(is_ready ? 1 : 0);
    return buffer;
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

std::vector<uint8_t> serialize_leave_game(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_LEAVE_GAME);
    push_uint16(buffer, game_id);
    return buffer;
}

std::vector<uint8_t> serialize_string(const std::string& str) {
    std::vector<uint8_t> buffer;
    LobbyProtocol::push_uint16(buffer, str.size());
    buffer.insert(buffer.end(), str.begin(), str.end());
    return buffer;
}


// Serializar petición de lista de juegos
std::vector<uint8_t> serialize_list_games() {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_LIST_GAMES);
    return buffer;
}

// Serializar petición de crear juego
std::vector<uint8_t> serialize_create_game(const std::string& game_name, uint8_t max_players, uint8_t num_races) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_CREATE_GAME);
    push_uint16(buffer, game_name.length());
    for (char c : game_name) {
        buffer.push_back(static_cast<uint8_t>(c));
    }
    buffer.push_back(max_players);
    buffer.push_back(num_races);
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



    /*[MSG_CITY_MAPS][num_ciudades] [len_ciudad][nombre_ciudad] [num_carreras] [len_yaml][nombre_yaml] [len_png][nombre_png]*/
    std::vector<uint8_t> serialize_city_maps(const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>& maps) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_CITY_MAPS);

    LobbyProtocol::push_uint16(buffer, maps.size()); // número de ciudades

    for (const auto& [city_name, tracks] : maps) {
        LobbyProtocol::push_uint16(buffer, city_name.size());
        buffer.insert(buffer.end(), city_name.begin(), city_name.end());

        LobbyProtocol::push_uint16(buffer, tracks.size());
        for (const auto& [yaml_file, png_file] : tracks) {
            LobbyProtocol::push_uint16(buffer, yaml_file.size());
            buffer.insert(buffer.end(), yaml_file.begin(), yaml_file.end());

            LobbyProtocol::push_uint16(buffer, png_file.size());
            buffer.insert(buffer.end(), png_file.begin(), png_file.end());
        }
    }

    return buffer;
}

    /**[MSG_CAR_CHOSEN][len_nombre][nombre][len_tipo][tipo]**/
/*std::vector<uint8_t> serialize_car_chosen(const std::string& car_name, const std::string& car_type) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_CAR_CHOSEN);
    LobbyProtocol::push_uint16(buffer, car_name.size());
    buffer.insert(buffer.end(), car_name.begin(), car_name.end());
    LobbyProtocol::push_uint16(buffer, car_type.size());
    buffer.insert(buffer.end(), car_type.begin(), car_type.end());
    return buffer;
}
*/
std::vector<uint8_t> serialize_game_started(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_START_GAME);
    LobbyProtocol::push_uint16(buffer, game_id);
    return buffer;
}

std::vector<uint8_t> serialize_car_selected_ack(const std::string& car_name, const std::string& car_type) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_CAR_SELECTED_ACK);
    push_uint16(buffer, car_name.size());
    buffer.insert(buffer.end(), car_name.begin(), car_name.end());
    push_uint16(buffer, car_type.size());
    buffer.insert(buffer.end(), car_type.begin(), car_type.end());
    return buffer;
}

std::vector<uint8_t> serialize_select_car(const std::string& car_name, const std::string& car_type) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_SELECT_CAR);  // 
    push_uint16(buffer, car_name.size());
    buffer.insert(buffer.end(), car_name.begin(), car_name.end());
    push_uint16(buffer, car_type.size());
    buffer.insert(buffer.end(), car_type.begin(), car_type.end());
    return buffer;
}

std::vector<uint8_t> serialize_start_game(uint16_t game_id) {
    std::vector<uint8_t> buffer;
    buffer.push_back(MSG_START_GAME);
    push_uint16(buffer, game_id);
    return buffer;
}



} // namespace LobbyProtocol
