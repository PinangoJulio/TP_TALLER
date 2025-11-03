#include "game_room.h"
#include <algorithm>
#include <iostream>

GameRoom::GameRoom(uint16_t id, const std::string& name, const std::string& creator, uint8_t max_players)
    : game_id(id), game_name(name), host_username(creator), max_players(max_players), state(GameState::WAITING) {
    
    // El creador automáticamente se une como primer jugador y host
    players.push_back(creator);
    std::cout << "[GameRoom " << game_id << "] Created by '" << creator 
              << "' (HOST) (1/" << static_cast<int>(max_players) << ")" << std::endl;
}

bool GameRoom::add_player(const std::string& username) {
    if (is_full() || state == GameState::STARTED) {
        return false;
    }
    
    // Verificar si el jugador ya está en la partida
    if (has_player(username)) {
        std::cout << "[GameRoom " << game_id << "] Player '" << username 
                  << "' is already in the game!" << std::endl;
        return true;  // No es un error, simplemente ya está
    }
    
    players.push_back(username);
    std::cout << "[GameRoom " << game_id << "] Player '" << username 
              << "' joined (" << players.size() << "/" 
              << static_cast<int>(max_players) << ")" << std::endl;
    
    // Actualizar estado si ahora hay suficientes jugadores
    if (players.size() >= 2 && state == GameState::WAITING) {
        state = GameState::READY;
        std::cout << "[GameRoom " << game_id << "] Game is now READY to start!" << std::endl;
    }
    
    return true;
}

bool GameRoom::remove_player(const std::string& username) {
    auto it = std::find(players.begin(), players.end(), username);
    if (it == players.end()) {
        return false;  // Jugador no encontrado
    }
    
    bool was_host = (username == host_username);
    
    players.erase(it);
    std::cout << "[GameRoom " << game_id << "] Player '" << username 
              << "' left (" << players.size() << "/" 
              << static_cast<int>(max_players) << ")" << std::endl;
    
    // Si era el host, promover a otro
    if (was_host && !players.empty()) {
        promote_new_host();
    }
    
    // Actualizar estado si quedan menos de 2 jugadores
    if (players.size() < 2) {
        state = GameState::WAITING;
        std::cout << "[GameRoom " << game_id << "] Not enough players, state changed to WAITING" << std::endl;
    }
    
    return true;
}

void GameRoom::promote_new_host() {
    if (players.empty()) {
        host_username.clear();
        return;
    }
    
    host_username = players[0];  // El primer jugador se convierte en host
    std::cout << "[GameRoom " << game_id << "] '" << host_username 
              << "' is now the HOST" << std::endl;
}

bool GameRoom::is_host(const std::string& username) const {
    return username == host_username;
}

bool GameRoom::has_player(const std::string& username) const {
    return std::find(players.begin(), players.end(), username) != players.end();
}

bool GameRoom::is_full() const {
    return players.size() >= max_players;
}

bool GameRoom::is_ready() const {
    return players.size() >= 2 && state != GameState::STARTED;
}

bool GameRoom::start(const std::string& requester) {
    // Solo el host puede iniciar la partida
    if (!is_host(requester)) {
        std::cout << "[GameRoom " << game_id << "] '" << requester 
                  << "' tried to start game but is not the host!" << std::endl;
        return false;
    }
    
    // Verificar que haya suficientes jugadores
    if (!is_ready()) {
        std::cout << "[GameRoom " << game_id << "] Cannot start: not enough players!" << std::endl;
        return false;
    }
    
    state = GameState::STARTED;
    std::cout << "[GameRoom " << game_id << "] Game STARTED by host '" 
              << requester << "' with " << players.size() << " players!" << std::endl;
    return true;
}

bool GameRoom::kick_player(const std::string& host, const std::string& target) {
    // Solo el host puede expulsar
    if (!is_host(host)) {
        return false;
    }
    
    // No puede expulsarse a sí mismo
    if (host == target) {
        return false;
    }
    
    return remove_player(target);
}
