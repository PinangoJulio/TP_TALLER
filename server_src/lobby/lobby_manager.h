#ifndef LOBBY_MANAGER_H
#define LOBBY_MANAGER_H

#include <map>
#include <string>
#include <memory>
#include <cstdint>
#include "game_room.h"

class LobbyManager {
private:
    std::map<uint16_t, std::unique_ptr<GameRoom>> games;
    std::map<std::string, uint16_t> player_to_game;
    uint16_t next_game_id;

public:
    LobbyManager();
    
    uint16_t create_game(const std::string& game_name, const std::string& host, uint8_t max_players);
    bool join_game(uint16_t game_id, const std::string& username);
    bool leave_game(const std::string& username);
    bool start_game(uint16_t game_id, const std::string& username);
    
    bool is_player_in_game(const std::string& username) const;
    uint16_t get_player_game(const std::string& username) const;
    bool is_game_ready(uint16_t game_id) const;
    
    const std::map<uint16_t, std::unique_ptr<GameRoom>>& get_all_games() const;
};

#endif // LOBBY_MANAGER_H