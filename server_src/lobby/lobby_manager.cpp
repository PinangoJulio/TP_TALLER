#include "lobby_manager.h"
#include <iostream>

LobbyManager::LobbyManager() : next_game_id(1) {}

uint16_t LobbyManager::create_game(const std::string& game_name, const std::string& host, uint8_t max_players) {
    if (is_player_in_game(host)) {
        std::cout << "[LobbyManager] Player '" << host << "' is already in a game!" << std::endl;
        return 0;
    }
    
    uint16_t game_id = next_game_id++;
    
    auto room = std::make_unique<GameRoom>(game_id, game_name, host, max_players);
    
    // 🔥 Registrar callback de broadcast en la sala
    room->set_broadcast_callback([this, game_id](const std::vector<uint8_t>& buffer) {
        this->broadcast_to_game(game_id, buffer);
    });
    
    games[game_id] = std::move(room);
    player_to_game[host] = game_id;
    
    std::cout << "[LobbyManager] Game '" << game_name << "' created with ID " << game_id << " by '" << host << "'" << std::endl;
    
    return game_id;
}

bool LobbyManager::join_game(uint16_t game_id, const std::string& username) {
    // 🔥 DEBUG: Verificar estado antes de validar
    std::cout << "[LobbyManager] join_game() called: username=" << username 
              << ", game_id=" << game_id << std::endl;
    std::cout << "[LobbyManager] is_player_in_game(" << username << ")? " 
              << (is_player_in_game(username) ? "YES" : "NO") << std::endl;
    
    if (is_player_in_game(username)) {
        std::cout << "[LobbyManager] Player '" << username << "' is already in game " 
                  << player_to_game[username] << std::endl;
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
    
    // 🔥 DESREGISTRAR SOCKET **ANTES** DE ELIMINAR DE LA SALA
    unregister_player_socket(game_id, username);
    
    auto game_it = games.find(game_id);
    if (game_it != games.end()) {
        game_it->second->remove_player(username);
        
        // Si quedan menos de 2 jugadores, eliminar la partida
        if (game_it->second->get_player_count() == 0) {  // 🔥 CAMBIO: == 0 en vez de < 2
            std::cout << "[LobbyManager] Game " << game_id << " is empty, deleting..." << std::endl;
            player_sockets.erase(game_id);
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

// ===============================================
// 🔥 MÉTODOS DE BROADCAST
// ===============================================

void LobbyManager::register_player_socket(uint16_t game_id, const std::string& username, Socket& socket) {
    player_sockets[game_id][username] = &socket;
    
    std::cout << "[LobbyManager] Registered socket for player '" << username 
              << "' in game " << game_id << std::endl;
}

void LobbyManager::unregister_player_socket(uint16_t game_id, const std::string& username) {
    auto it = player_sockets.find(game_id);
    if (it != player_sockets.end()) {
        it->second.erase(username);
        
        std::cout << "[LobbyManager] Unregistered socket for player '" << username 
                  << "' in game " << game_id << std::endl;
        
        if (it->second.empty()) {
            player_sockets.erase(it);
        }
    }
}

void LobbyManager::broadcast_to_game(uint16_t game_id, const std::vector<uint8_t>& buffer) {
    auto it = player_sockets.find(game_id);
    if (it == player_sockets.end()) {
        std::cout << "[LobbyManager] No sockets registered for game " << game_id 
                  << " (may not have joined yet)" << std::endl;
        return;
    }
    
    std::cout << "[LobbyManager] Broadcasting to " << it->second.size() 
              << " players in game " << game_id << std::endl;
    
    for (auto& [username, socket_ptr] : it->second) {
        try {
            if (!socket_ptr) {
                std::cerr << "[LobbyManager] ❌ Null socket for " << username << std::endl;
                continue;
            }
            
            socket_ptr->sendall(buffer.data(), buffer.size());
            
            std::cout << "[LobbyManager] ✅ Sent to " << username << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[LobbyManager] ❌ Error broadcasting to " << username 
                      << ": " << e.what() << std::endl;
        }
    }
}