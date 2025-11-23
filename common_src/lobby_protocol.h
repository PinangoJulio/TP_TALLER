#ifndef LOBBY_PROTOCOL_H
#define LOBBY_PROTOCOL_H

#include <cstdint>
#include <string>
#include <utility>
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
std::vector<uint8_t> serialize_string(const std::string& str);
std::vector<uint8_t> serialize_list_games();
std::vector<uint8_t> serialize_create_game(const std::string& game_name, uint8_t max_players,
                                           uint8_t num_races);
std::vector<uint8_t> serialize_join_game(uint16_t game_id);
std::vector<uint8_t> serialize_welcome(const std::string& message);
std::vector<uint8_t> serialize_games_list(const std::vector<GameInfo>& games);
std::vector<uint8_t> serialize_game_created(uint16_t game_id);
std::vector<uint8_t> serialize_game_joined(uint16_t game_id);
std::vector<uint8_t> serialize_error(LobbyErrorCode error_code, const std::string& message);
std::vector<uint8_t> serialize_city_maps(
    const std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>&
        maps);
std::vector<uint8_t> serialize_game_started(uint16_t game_id);
std::vector<uint8_t> serialize_leave_game(uint16_t game_id);
std::vector<uint8_t> serialize_car_selected_ack(const std::string& car_name,
                                                const std::string& car_type);
std::vector<uint8_t> serialize_select_car(const std::string& car_name, const std::string& car_type);

// NUEVAS FUNCIONES DE SERIALIZACIÓN PARA NOTIFICACIONES DEL LOBBY:
std::vector<uint8_t> serialize_player_joined_notification(const std::string& username);
std::vector<uint8_t> serialize_player_left_notification(const std::string& username);
std::vector<uint8_t> serialize_player_ready_notification(const std::string& username, bool ready);
std::vector<uint8_t> serialize_car_selected_notification(const std::string& username,
                                                         const std::string& car_name,
                                                         const std::string& car_type);
std::vector<uint8_t> serialize_player_ready(bool is_ready);

}  // namespace LobbyProtocol

#endif  // LOBBY_PROTOCOL_H
