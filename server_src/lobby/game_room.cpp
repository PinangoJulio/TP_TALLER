#include "game_room.h"
#include <iostream>

GameRoom::GameRoom(uint16_t id, const std::string& name, const std::string& host, uint8_t max_players)
    : game_id(id), 
      game_name(name), 
      max_players(max_players), 
      state(RoomState::WAITING) { // Corregido: GameState a RoomState
    
    LobbyPlayerInfo host_info; // Corregido: PlayerInfo a LobbyPlayerInfo
    host_info.is_host = true;
    host_info.is_ready = false;
    host_info.car_index = 0;
    
    players[host] = host_info;
    
    std::cout << "[GameRoom " << game_id << "] Created by '" << host << "' (HOST) (" 
              << players.size() << "/" << static_cast<int>(max_players) << ")" << std::endl;
}

bool GameRoom::add_player(const std::string& username) {
    if (players.size() >= max_players) {
        return false;
    }
    
    if (state == RoomState::STARTED) { // Corregido: GameState a RoomState
        return false;
    }
    
    if (players.find(username) != players.end()) {
        std::cout << "[GameRoom " << game_id << "] Player '" << username << "' is already in the game!" << std::endl;
        return false;
    }
    
    LobbyPlayerInfo info; // Corregido: PlayerInfo a LobbyPlayerInfo
    info.is_host = false;
    info.is_ready = false;
    info.car_index = 0;
    
    players[username] = info;
    
    std::cout << "[GameRoom " << game_id << "] Player '" << username << "' joined (" 
              << players.size() << "/" << static_cast<int>(max_players) << ")" << std::endl;
    
    if (players.size() >= 2 && state == RoomState::WAITING) { // Corregido: GameState a RoomState
        state = RoomState::READY; // Corregido: GameState a RoomState
        std::cout << "[GameRoom " << game_id << "] Enough players, state changed to READY" << std::endl;
    }
    
    return true;
}

void GameRoom::remove_player(const std::string& username) {
    auto it = players.find(username);
    if (it == players.end()) {
        return;
    }
    
    std::cout << "[GameRoom " << game_id << "] Player '" << username << "' left ";
    
    // Si era el host, transferir a otro jugador
    if (it->second.is_host && players.size() > 1) {
        players.erase(it);
        
        // Asignar nuevo host (el primero disponible)
        auto new_host = players.begin();
        new_host->second.is_host = true;
        
        std::cout << "(" << players.size() << "/" << static_cast<int>(max_players) << ") "
                  << "[NEW HOST: '" << new_host->first << "']" << std::endl;
    } else {
        players.erase(it);
        std::cout << "(" << players.size() << "/" << static_cast<int>(max_players) << ")" << std::endl;
    }
    
    // Cambiar estado si quedan menos de 2 jugadores
    if (players.size() < 2 && state == RoomState::READY) { // Corregido: GameState a RoomState
        state = RoomState::WAITING; // Corregido: GameState a RoomState
        std::cout << "[GameRoom " << game_id << "] Not enough players, state changed to WAITING" << std::endl;
    }
}

bool GameRoom::has_player(const std::string& username) const {
    return players.find(username) != players.end();
}

bool GameRoom::is_host(const std::string& username) const {
    auto it = players.find(username);
    if (it == players.end()) {
        return false;
    }
    return it->second.is_host;
}

bool GameRoom::is_ready() const {
    return state == RoomState::READY && players.size() >= 2; // Corregido: GameState a RoomState
}

bool GameRoom::is_started() const {
    return state == RoomState::STARTED; // Corregido: GameState a RoomState
}

void GameRoom::start() {
    if (state != RoomState::READY) { // Corregido: GameState a RoomState
        return;
    }
    
    state = RoomState::STARTED; // Corregido: GameState a RoomState
    std::cout << "[GameRoom " << game_id << "] Game started!" << std::endl;
}

bool GameRoom::set_player_car(const std::string& username, uint8_t car_index) {
    auto it = players.find(username);
    if (it == players.end()) {
        return false;
    }
    
    it->second.car_index = car_index;
    std::cout << "[GameRoom " << game_id << "] Player '" << username 
              << "' selected car " << static_cast<int>(car_index) << std::endl;
    return true;
}

uint8_t GameRoom::get_player_car(const std::string& username) const {
    auto it = players.find(username);
    if (it == players.end()) {
        return 0;
    }
    return it->second.car_index;
}

bool GameRoom::all_players_selected_car() const {
    for (const auto& pair : players) {
        if (pair.second.car_index == 0) {
            return false;
        }
    }
    return true;
}

uint16_t GameRoom::get_game_id() const {
    return game_id;
}

std::string GameRoom::get_game_name() const {
    return game_name;
}

uint8_t GameRoom::get_player_count() const {
    return players.size();
}

uint8_t GameRoom::get_max_players() const {
    return max_players;
}

const std::map<std::string, LobbyPlayerInfo>& GameRoom::get_players() const {
    return players;
}