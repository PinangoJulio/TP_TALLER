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
    uint16_t next_game_id;
    std::mutex mtx;  // Protege el acceso a la lista de partidas

public:
    LobbyManager();

    // Crear una nueva partida
    uint16_t create_game(const std::string& game_name, uint8_t max_players);

    // Unirse a una partida existente
    bool join_game(uint16_t game_id, const std::string& username);

    // Obtener lista de partidas disponibles
    std::vector<GameInfo> get_games_list();

    // Verificar si una partida está lista para comenzar
    bool is_game_ready(uint16_t game_id);

    // No se puede copiar el LobbyManager
    LobbyManager(const LobbyManager&) = delete;
    LobbyManager& operator=(const LobbyManager&) = delete;
};

#endif // LOBBY_MANAGER_H
