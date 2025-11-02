#include "lobby_manager.h"
#include <iostream>
#include <cstring>

LobbyManager::LobbyManager() : next_game_id(1) {}

uint16_t LobbyManager::create_game(const std::string& game_name, uint8_t max_players) {
    std::lock_guard<std::mutex> lock(mtx);
    
    uint16_t game_id = next_game_id++;
    auto game = std::make_unique<GameRoom>(game_id, game_name, max_players);
    
    games[game_id] = std::move(game);
    
    std::cout << "[LobbyManager] Game '" << game_name 
              << "' created with ID " << game_id << std::endl;
    
    return game_id;
}

bool LobbyManager::join_game(uint16_t game_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        std::cout << "[LobbyManager] Game " << game_id << " not found" << std::endl;
        return false;
    }
    
    return it->second->add_player(username);
}

std::vector<GameInfo> LobbyManager::get_games_list() {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::vector<GameInfo> games_list;
    
    for (const auto& pair : games) {
        const GameRoom* game = pair.second.get();
        
        GameInfo info;
        info.game_id = game->get_id();
        
        // Copiar el nombre (con seguridad)
        std::strncpy(info.game_name, game->get_name().c_str(), sizeof(info.game_name) - 1);
        info.game_name[sizeof(info.game_name) - 1] = '\0';
        
        info.current_players = game->get_current_players();
        info.max_players = game->get_max_players();
        info.is_started = game->has_started();
        
        games_list.push_back(info);
    }
    
    return games_list;
}

bool LobbyManager::is_game_ready(uint16_t game_id) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    return it->second->is_ready();
}
