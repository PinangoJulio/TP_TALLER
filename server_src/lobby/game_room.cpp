#include "game_room.h"

#include <iostream>

#include "../../common_src/lobby_protocol.h"

GameRoom::GameRoom(uint16_t id, const std::string& name, const std::string& creator,
                   uint8_t max_players)
    : game_id(id), game_name(name), creator_username(creator), max_players(max_players),
      state(RoomState::WAITING) {
    LobbyPlayerInfo creator_info;
    creator_info.is_ready = false;
    creator_info.car_name = "";
    creator_info.car_type = "";

    players[creator] = creator_info;

    std::cout << "[GameRoom " << game_id << "] Created by '" << creator << "'" << std::endl;
}

bool GameRoom::add_player(const std::string& username) {
    if (players.size() >= max_players)
        return false;
    if (state == RoomState::STARTED)
        return false;
    if (players.find(username) != players.end())
        return false;

    LobbyPlayerInfo info;
    info.is_ready = false;
    info.car_name = "";
    info.car_type = "";

    players[username] = info;

    std::cout << "[GameRoom " << game_id << "] Player '" << username << "' joined ("
              << players.size() << "/" << static_cast<int>(max_players) << ")" << std::endl;

    if (players.size() >= 2 && state == RoomState::WAITING) {
        state = RoomState::READY;
    }

    return true;
}

void GameRoom::remove_player(const std::string& username) {
    auto it = players.find(username);
    if (it == players.end())
        return;

    players.erase(it);

    std::cout << "[GameRoom " << game_id << "] Player '" << username << "' left" << std::endl;

    if (broadcast_callback) {
        auto buffer = LobbyProtocol::serialize_player_left_notification(username);
        broadcast_callback(buffer);
    }

    if (players.size() < 2 && state == RoomState::READY) {
        state = RoomState::WAITING;
    }
}

bool GameRoom::set_player_ready(const std::string& username, bool ready) {
    auto it = players.find(username);
    if (it == players.end())
        return false;

    if (ready && it->second.car_name.empty()) {
        std::cout << "[GameRoom " << game_id << "] Player '" << username
                  << "' cannot be ready without selecting a car" << std::endl;
        return false;
    }

    it->second.is_ready = ready;

    std::cout << "[GameRoom " << game_id << "] Player '" << username << "' is now "
              << (ready ? "READY" : "NOT READY") << std::endl;

    // --- CAMBIO: Auto-start si todos están listos ---
    if (state == RoomState::READY && all_players_ready()) {
        std::cout << "[GameRoom " << game_id << "] All players ready! Auto-starting game..." << std::endl;
        start();
    }
    // ------------------------------------------------

    return true;
}

bool GameRoom::is_player_ready(const std::string& username) const {
    auto it = players.find(username);
    return it != players.end() && it->second.is_ready;
}

bool GameRoom::all_players_ready() const {
    if (players.size() < 2)
        return false;

    for (const auto& [username, info] : players) {
        (void)username;  // Unused
        if (!info.is_ready)
            return false;
    }
    return true;
}

bool GameRoom::set_player_car(const std::string& username, const std::string& car_name,
                              const std::string& car_type) {
    auto it = players.find(username);
    if (it == players.end()) {
        std::cerr << "[GameRoom] ❌ Player '" << username << "' NOT FOUND in game " << game_id
                  << std::endl;
        return false;
    }

    it->second.car_name = car_name;
    it->second.car_type = car_type;

    std::cout << "[GameRoom] ✅ Player '" << username << "' car saved: " << car_name << " ("
              << car_type << ")" << std::endl;

    return true;
}

bool GameRoom::is_ready() const {
    return state == RoomState::READY && players.size() >= 2 && all_players_ready();
}

bool GameRoom::is_started() const {
    return state == RoomState::STARTED;
}

void GameRoom::start() {
    if (state != RoomState::READY)
        return;
    
    // Doble verificación por seguridad
    if (!all_players_ready()) {
        std::cout << "[GameRoom " << game_id << "] Cannot start: not all players ready" << std::endl;
        return;
    }

    state = RoomState::STARTED;
    std::cout << "[GameRoom " << game_id << "] Game started!" << std::endl;

    // --- CAMBIO: Notificar a todos los clientes ---
    if (broadcast_callback) {
        // Serializamos el mensaje de START (asegúrate de que serialize_game_started exista en LobbyProtocol)
        auto buffer = LobbyProtocol::serialize_game_started(game_id);
        broadcast_callback(buffer);
    }
    // ----------------------------------------------
}

bool GameRoom::has_player(const std::string& username) const {
    return players.find(username) != players.end();
}

std::string GameRoom::get_player_car(const std::string& username) const {
    auto it = players.find(username);
    return it != players.end() ? it->second.car_name : "";
}

bool GameRoom::all_players_selected_car() const {
    for (const auto& [username, info] : players) {
        (void)username;  // Unused
        if (info.car_name.empty())
            return false;
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
