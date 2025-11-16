#ifndef LOBBY_MANAGER_H
#define LOBBY_MANAGER_H

#include <map>
#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <vector>
#include "game_room.h"
#include "../../common_src/socket.h"

class LobbyManager {
private:
    std::map<uint16_t, std::unique_ptr<GameRoom>> games;
    std::map<std::string, uint16_t> player_to_game;
    uint16_t next_game_id;
    
    std::map<uint16_t, std::map<std::string, Socket*>> player_sockets;

public:
    LobbyManager();
    
    uint16_t create_game(const std::string& game_name, const std::string& host, uint8_t max_players);
    bool join_game(uint16_t game_id, const std::string& username);
    bool leave_game(const std::string& username);
    bool start_game(uint16_t game_id, const std::string& username);
    
    bool is_player_in_game(const std::string& username) const;
    uint16_t get_player_game(const std::string& username) const;
    bool is_game_ready(uint16_t game_id) const;
    
    // ✅ RETORNAR POR REFERENCIA CONSTANTE
    const std::map<uint16_t, std::unique_ptr<GameRoom>>& get_all_games() const;
    
    void register_player_socket(uint16_t game_id, const std::string& username, Socket& socket);
    void unregister_player_socket(uint16_t game_id, const std::string& username);
    void broadcast_to_game(uint16_t game_id, const std::vector<uint8_t>& buffer);
};

#endif // LOBBY_MANAGER_H