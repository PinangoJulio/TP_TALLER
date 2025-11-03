#include "lobby_manager.h"
#include <iostream>
#include <cstring>

LobbyManager::LobbyManager() : next_game_id(1) {}

uint16_t LobbyManager::create_game(const std::string& game_name, const std::string& creator, uint8_t max_players) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Verificar si el jugador ya está en una partida
    if (player_to_game.find(creator) != player_to_game.end()) {
        std::cout << "[LobbyManager] Player '" << creator 
                  << "' is already in a game!" << std::endl;
        return 0;  // Indica error
    }
    
    uint16_t game_id = next_game_id++;
    auto game = std::make_unique<GameRoom>(game_id, game_name, creator, max_players);
    
    games[game_id] = std::move(game);
    player_to_game[creator] = game_id;
    
    std::cout << "[LobbyManager] Game '" << game_name 
              << "' created with ID " << game_id << " by '" << creator << "'" << std::endl;
    
    return game_id;
}

bool LobbyManager::join_game(uint16_t game_id, const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Verificar si el jugador ya está en una partida
    if (player_to_game.find(username) != player_to_game.end()) {
        std::cout << "[LobbyManager] Player '" << username 
                  << "' is already in game " << player_to_game[username] << std::endl;
        return false;
    }
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        std::cout << "[LobbyManager] Game " << game_id << " not found" << std::endl;
        return false;
    }
    
    bool success = it->second->add_player(username);
    if (success) {
        player_to_game[username] = game_id;
    }
    
    return success;
}

bool LobbyManager::leave_game(const std::string& username) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto player_it = player_to_game.find(username);
    if (player_it == player_to_game.end()) {
        return false;  // Jugador no está en ninguna partida
    }
    
    uint16_t game_id = player_it->second;
    auto game_it = games.find(game_id);
    if (game_it == games.end()) {
        player_to_game.erase(player_it);
        return false;
    }
    
    game_it->second->remove_player(username);
    player_to_game.erase(player_it);
    
    // Auto-destruir partida si quedan menos de 2 jugadores
    if (game_it->second->get_current_players() < 2) {
        std::cout << "[LobbyManager] Game " << game_id 
                  << " has less than 2 players, deleting..." << std::endl;
        
        // Remover a todos los jugadores restantes del tracking
        for (const auto& player : game_it->second->get_players()) {
            player_to_game.erase(player);
        }
        
        games.erase(game_it);
    }
    
    return true;
}

bool LobbyManager::start_game(uint16_t game_id, const std::string& requester) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    return it->second->start(requester);
}

bool LobbyManager::kick_player(uint16_t game_id, const std::string& host, const std::string& target) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    bool success = it->second->kick_player(host, target);
    if (success) {
        player_to_game.erase(target);
    }
    
    return success;
}

bool LobbyManager::delete_game(uint16_t game_id, const std::string& requester) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    // Solo el host puede eliminar la partida
    if (!it->second->is_host(requester)) {
        return false;
    }
    
    // Remover a todos los jugadores del tracking
    for (const auto& player : it->second->get_players()) {
        player_to_game.erase(player);
    }
    
    games.erase(it);
    std::cout << "[LobbyManager] Game " << game_id 
              << " deleted by host '" << requester << "'" << std::endl;
    
    return true;
}

std::vector<GameInfo> LobbyManager::get_games_list() {
    std::lock_guard<std::mutex> lock(mtx);
    
    std::vector<GameInfo> games_list;
    
    for (const auto& pair : games) {
        const GameRoom* game = pair.second.get();
        
        // Solo mostrar partidas que no han iniciado
        if (game->has_started()) {
            continue;
        }
        
        GameInfo info;
        info.game_id = game->get_id();
        
        std::strncpy(info.game_name, game->get_name().c_str(), sizeof(info.game_name) - 1);
        info.game_name[sizeof(info.game_name) - 1] = '\0';
        
        info.current_players = game->get_current_players();
        info.max_players = game->get_max_players();
        info.is_started = game->has_started();
        
        games_list.push_back(info);
    }
    
    return games_list;
}

bool LobbyManager::is_player_in_game(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return player_to_game.find(username) != player_to_game.end();
}

uint16_t LobbyManager::get_player_game(const std::string& username) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = player_to_game.find(username);
    return (it != player_to_game.end()) ? it->second : 0;
}

bool LobbyManager::is_game_ready(uint16_t game_id) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = games.find(game_id);
    if (it == games.end()) {
        return false;
    }
    
    return it->second->is_ready();
}
