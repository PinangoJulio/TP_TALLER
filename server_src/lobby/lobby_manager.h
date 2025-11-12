#ifndef LOBBY_MANAGER_H
#define LOBBY_MANAGER_H

#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <string>

#include "game_room.h"
#include "../../common_src/lobby_protocol.h"

class LobbyManager {
private:
    std::map<uint16_t, std::unique_ptr<GameRoom>> games;
    std::map<std::string, uint16_t> player_to_game;
    uint16_t next_game_id;
    std::mutex mtx;

public:
    LobbyManager();

    // Crear una nueva partida
    uint16_t create_game(const std::string& game_name, const std::string& creator, uint8_t max_players);

    // Unirse a una partida existente
    bool join_game(uint16_t game_id, const std::string& username);

    // Abandonar la partida actual
    bool leave_game(const std::string& username);

    // Iniciar partida (solo host)
    bool start_game(uint16_t game_id, const std::string& requester);

    // Expulsar jugador (solo host)
    bool kick_player(uint16_t game_id, const std::string& host, const std::string& target);

    // Eliminar partida (solo host o auto-destrucción)
    bool delete_game(uint16_t game_id, const std::string& requester);

    // Obtener lista de partidas disponibles
    std::vector<GameInfo> get_games_list();

    // Verificar si un jugador ya está en una partida
    bool is_player_in_game(const std::string& username) const;

    // Obtener la partida de un jugador
    uint16_t get_player_game(const std::string& username) const;

    // Verificar si una partida está lista para comenzar
    bool is_game_ready(uint16_t game_id);

    // ✅ AGREGADO: Gestión de autos
    bool set_player_car(uint16_t game_id, const std::string& username, uint8_t car_index);
    uint8_t get_player_car(uint16_t game_id, const std::string& username) const;

    // No se puede copiar el LobbyManager
    LobbyManager(const LobbyManager&) = delete;
    LobbyManager& operator=(const LobbyManager&) = delete;
};

#endif // LOBBY_MANAGER_H