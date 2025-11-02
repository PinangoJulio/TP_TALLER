#include "game_room.h"
#include <iostream>

GameRoom::GameRoom(uint16_t id, const std::string& name, uint8_t max_players)
    : game_id(id), game_name(name), max_players(max_players), is_started(false) {}

bool GameRoom::add_player(const std::string& username) {
    if (is_full() || is_started) {
        return false;
    }
    
    players.push_back(username);
    std::cout << "[GameRoom " << game_id << "] Player '" << username 
              << "' joined (" << players.size() << "/" 
              << static_cast<int>(max_players) << ")" << std::endl;
    return true;
}

bool GameRoom::is_full() const {
    return players.size() >= max_players;
}

bool GameRoom::is_ready() const {
    // Una partida está lista cuando tiene al menos 2 jugadores
    return players.size() >= 2;
}

void GameRoom::start() {
    is_started = true;
    std::cout << "[GameRoom " << game_id << "] Game STARTED with " 
              << players.size() << " players!" << std::endl;
}
