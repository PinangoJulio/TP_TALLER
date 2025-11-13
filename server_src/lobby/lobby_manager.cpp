#include "lobby_manager.h"
#include <iostream>

LobbyManager::LobbyManager() : next_game_id(1) {}

uint16_t LobbyManager::create_game(const std::string& game_name, const std::string& host, uint8_t max_players) {
    if (is_player_in_game(host)) {
        std::cout << "[LobbyManager] Player '" << host << "' is already in a game!" << std::endl;
        return 0;
    }
    
    uint16_t game_id = next_game_id++;
    
    games[game_id] = std::make_unique<GameRoom>(game_id, game_name, host, max_players);
    player_to_game[host] = game_id;
    
    std::cout << "[LobbyManager] Game '" << game_name << "' created with ID " << game_id << " by '" << host << "'" << std::endl;
    
    return game_id;
}

bool LobbyManager::join_game(uint16_t game_id, const std::string& username) {
    if (is_player_in_game(username)) {
        std::cout << "[LobbyManager] Player '" << username << "' is already in a game!" << std::endl;
        return false;
    }
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        std::cout << "[LobbyManager] Game " << game_id << " not found" << std::endl;
        return false;
    }
    
    if (!it->second->add_player(username)) {
        return false;
    }
    
    player_to_game[username] = game_id;
    return true;
}

bool LobbyManager::leave_game(const std::string& username) {
    auto it = player_to_game.find(username);
    if (it == player_to_game.end()) {
        return false;
    }
    
    uint16_t game_id = it->second;
    player_to_game.erase(it);
    
    auto game_it = games.find(game_id);
    if (game_it != games.end()) {
        game_it->second->remove_player(username);
        
        // Si quedan menos de 2 jugadores, eliminar la partida
        if (game_it->second->get_player_count() < 2) {
            std::cout << "[LobbyManager] Game " << game_id << " has less than 2 players, deleting..." << std::endl;
            games.erase(game_it);
        }
    }
    
    return true;
}

bool LobbyManager::start_game(uint16_t game_id, const std::string& username) {
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    if (!it->second->is_host(username)) {
        std::cout << "[LobbyManager] Player '" << username << "' is not the host of game " << game_id << std::endl;
        return false;
    }
    
    if (!it->second->is_ready()) {
        std::cout << "[LobbyManager] Game " << game_id << " is not ready to start" << std::endl;
        return false;
    }
    
    it->second->start();
    return true;
}

bool LobbyManager::is_player_in_game(const std::string& username) const {
    return player_to_game.find(username) != player_to_game.end();
}

uint16_t LobbyManager::get_player_game(const std::string& username) const {
    auto it = player_to_game.find(username);
    if (it == player_to_game.end()) {
        return 0;
    }
    return it->second;
}

bool LobbyManager::is_game_ready(uint16_t game_id) const {
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    return it->second->is_ready();
}

const std::map<uint16_t, std::unique_ptr<GameRoom>>& LobbyManager::get_all_games() const {
    return games;
}