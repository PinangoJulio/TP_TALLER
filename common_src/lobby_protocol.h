#ifndef LOBBY_PROTOCOL_H
#define LOBBY_PROTOCOL_H

#include <cstdint>
#include <string>
#include <vector>

#include "dtos.h"

// Estructura para información de una partida
struct GameInfo {
    uint16_t game_id;
    char game_name[32];
    uint8_t current_players;
    uint8_t max_players;
    bool is_started;
} __attribute__((packed));


namespace LobbyProtocol {
    // Funciones de serialización
    std::vector<uint8_t> serialize_username(const std::string& username);
    std::vector<uint8_t> serialize_list_games();
    std::vector<uint8_t> serialize_create_game(const std::string& game_name, uint8_t max_players);
    std::vector<uint8_t> serialize_join_game(uint16_t game_id);
    std::vector<uint8_t> serialize_welcome(const std::string& message);
    std::vector<uint8_t> serialize_games_list(const std::vector<GameInfo>& games);
    std::vector<uint8_t> serialize_game_created(uint16_t game_id);
    std::vector<uint8_t> serialize_game_joined(uint16_t game_id);
    std::vector<uint8_t> serialize_error(LobbyErrorCode error_code, const std::string& message);
    std::vector<uint8_t> serialize_game_started(uint16_t game_id);
    
    // ✅ AGREGADO: Nuevas funciones de serialización
    std::vector<uint8_t> serialize_select_car(uint8_t car_index);
    std::vector<uint8_t> serialize_start_game(uint16_t game_id);
    std::vector<uint8_t> serialize_leave_game(uint16_t game_id);
}

#endif // LOBBY_PROTOCOL_H
