#include "matches_monitor.h"

#include <cstring>
#include <iostream>
#include <utility>

// ============================================
// CREACIÓN Y GESTIÓN DE PARTIDAS
// ============================================

int MatchesMonitor::create_match(int max_players, const std::string& host_name, int player_id,
                                 Queue<GameState>& sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);

  
    if (player_to_match.find(host_name) != player_to_match.end()) {
        std::cerr << "[MatchesMonitor] Player '" << host_name << "' is already in match "
                  << player_to_match[host_name] << std::endl;
        return -1;  // Error: ya está en otra partida
    }

    int match_id = ++id_matches;

    std::unique_ptr<Match> match = std::make_unique<Match>(host_name, match_id, max_players);

  
    matches.emplace(match_id, std::move(match));

  
    auto& inserted_match = matches[match_id];

  
    inserted_match->set_broadcast_callback(
        [this, match_id](const std::vector<uint8_t>& buffer, int /*exclude_player_id*/) {
            this->broadcast_to_match(match_id, buffer, "");
        });

  
    inserted_match->add_player(player_id, host_name, sender_message_queue);

  
    player_to_match[host_name] = match_id;

    std::cout << "[MatchesMonitor]   Match created: ID=" << match_id << " Host=" << host_name
              << " (player_id=" << player_id << ")" << std::endl;

    return match_id;
}

bool MatchesMonitor::join_match(int match_id, const std::string& player_name, int player_id,
                                Queue<GameState>& sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);

    std::cout << "[MatchesMonitor] join_match() called: player=" << player_name
              << " (id=" << player_id << "), match_id=" << match_id << std::endl;

    
    if (player_to_match.find(player_name) != player_to_match.end()) {
        std::cerr << "[MatchesMonitor] Player '" << player_name << "' is already in match "
                  << player_to_match[player_name] << std::endl;
        return false;
    }

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " not found" << std::endl;
        return false;
    }

    auto& match = it->second;

    if (!match->can_player_join_match()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " is full or already started"
                  << std::endl;
        return false;
    }

    bool success = match->add_player(player_id, player_name, sender_message_queue);

    if (success) {
        player_to_match[player_name] = match_id;
        std::cout << "[MatchesMonitor]   " << player_name << " (id=" << player_id
                  << ") joined match " << match_id << std::endl;
    } else {
        std::cerr << "[MatchesMonitor]   Failed to add " << player_name << " to match " << match_id
                  << std::endl;
    }

    return success;
}

bool MatchesMonitor::leave_match(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = player_to_match.find(player_name);
    if (it == player_to_match.end()) {
        std::cout << "[MatchesMonitor] Player '" << player_name << "' is not in any match"
                  << std::endl;
        return false;
    }

    int match_id = it->second;

  
    unregister_player_socket(match_id, player_name);

    // Eliminar del lookup
    player_to_match.erase(it);

    auto match_it = matches.find(match_id);
    if (match_it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " not found (inconsistency)"
                  << std::endl;
        return false;
    }

    int player_id = match_it->second->get_player_id_by_name(player_name);
    if (player_id == -1) {
        std::cerr << "[MatchesMonitor] Player '" << player_name << "' not found in match "
                  << match_id << std::endl;
        return false;
    }

    // Eliminar del match
    match_it->second->remove_player(player_id);

    std::cout << "[MatchesMonitor]   " << player_name << " left match " << match_id << std::endl;

   
    if (match_it->second->is_empty()) {
        std::cout << "[MatchesMonitor] Match " << match_id << " is empty, deleting..." << std::endl;
        player_sockets.erase(match_id);
        matches.erase(match_it);
    }

    return true;
}

bool MatchesMonitor::leave_match_by_id(int player_id, int match_id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " no encontrado\n";
        return false;
    }

    it->second->remove_player(player_id);

    // Si quedó vacío, eliminar
    if (it->second->is_empty()) {
        std::cout << "[MatchesMonitor] Match " << match_id << " está vacío, eliminándolo...\n";
        matches.erase(it);
    }

    return true;
}

// ============================================
// CARRERAS
// ============================================

bool MatchesMonitor::add_races_to_match(int match_id, const std::vector<ServerRaceConfig>& races) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match no encontrado: " << match_id << std::endl;
        return false;
    }

    it->second->set_race_configs(races);

    std::cout << "[MatchesMonitor] Carreras agregadas a match " << match_id << std::endl;
    return true;
}

// LISTADO Y VALIDACIONES

std::vector<std::string> MatchesMonitor::get_race_paths(int match_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " not found" << std::endl;
        return {};
    }

    return it->second->get_race_yaml_paths();
}


std::vector<GameInfo> MatchesMonitor::list_available_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<GameInfo> result;

    for (auto& [id, match] : matches) {
        GameInfo info{};
        info.game_id = id;

        std::string name = match->get_match_name();
        strncpy(info.game_name, name.c_str(), sizeof(info.game_name) - 1);
        info.game_name[sizeof(info.game_name) - 1] = '\0';

        info.current_players = match->get_player_count();
        info.max_players = match->get_max_players();
        info.is_started = match->is_started();
        result.push_back(info);
    }

    return result;
}

bool MatchesMonitor::is_player_in_match(const std::string& player_name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return player_to_match.find(player_name) != player_to_match.end();
}

int MatchesMonitor::get_player_match(const std::string& player_name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = player_to_match.find(player_name);
    return (it != player_to_match.end()) ? it->second : -1;
}

bool MatchesMonitor::is_match_ready(int match_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = matches.find(match_id);
    return (it != matches.end()) && it->second->can_start();
}

// ============================================
// ACCIONES DE JUGADORES
// ============================================

bool MatchesMonitor::set_player_car(const std::string& player_name, const std::string& car_name,
                                    const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = player_to_match.find(player_name);
    if (it == player_to_match.end()) {
        std::cerr << "[MatchesMonitor] Jugador " << player_name << " no está en ningún match\n";
        return false;
    }

    int match_id = it->second;
    auto match_it = matches.find(match_id);
    if (match_it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " no encontrado\n";
        return false;
    }

    return match_it->second->set_player_car_by_name(player_name, car_name, car_type);
}

bool MatchesMonitor::set_player_ready(const std::string& player_name, bool ready) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = player_to_match.find(player_name);
    if (it == player_to_match.end()) {
        std::cerr << "[MatchesMonitor] Jugador " << player_name << " no está en ningún match\n";
        return false;
    }

    int match_id = it->second;
    auto match_it = matches.find(match_id);
    if (match_it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " no encontrado\n";
        return false;
    }

    return match_it->second->set_player_ready_by_name(player_name, ready);
}

// ============================================
// SNAPSHOT
// ============================================

std::map<int, PlayerLobbyInfo> MatchesMonitor::get_match_players_snapshot(int match_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        return {};
    }

    return it->second->get_players_snapshot();
}

// ============================================
// SOCKETS Y BROADCAST
// ============================================

void MatchesMonitor::register_player_socket(int match_id, const std::string& player_name,
                                            Socket& socket) {
    std::lock_guard<std::mutex> lock(mtx);

    player_sockets[match_id][player_name] = &socket;

    std::cout << "[MatchesMonitor] Socket registered: " << player_name << " in match " << match_id
              << std::endl;
}

void MatchesMonitor::unregister_player_socket(int match_id, const std::string& player_name) {
    // ⚠️ NO hacer lock aquí porque puede ser llamado desde leave_match que ya tiene lock

    auto it = player_sockets.find(match_id);
    if (it != player_sockets.end()) {
        it->second.erase(player_name);

        std::cout << "[MatchesMonitor] Socket unregistered: " << player_name << " from match "
                  << match_id << std::endl;

        if (it->second.empty()) {
            player_sockets.erase(it);
        }
    }
}

void MatchesMonitor::broadcast_to_match(int match_id, const std::vector<uint8_t>& buffer,
                                        const std::string& exclude_player) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = player_sockets.find(match_id);
    if (it == player_sockets.end()) {
        std::cout << "[MatchesMonitor] No hay sockets registrados para match " << match_id
                  << std::endl;
        return;
    }

    std::cout << "[MatchesMonitor] Broadcasting to match " << match_id
              << " (excluding: " << (exclude_player.empty() ? "none" : exclude_player) << ")"
              << std::endl;

    int sent_count = 0;
    for (const auto& [player_name, socket] : it->second) {
        if (player_name == exclude_player) {
            std::cout << "[MatchesMonitor]   Skipping " << player_name << " (sender)" << std::endl;
            continue;
        }

        try {
            if (!socket) {
                std::cerr << "[MatchesMonitor]   Null socket for " << player_name << std::endl;
                continue;
            }

            socket->sendall(buffer.data(), buffer.size());
            std::cout << "[MatchesMonitor]   Sent to " << player_name << std::endl;
            sent_count++;

        } catch (const std::exception& e) {
            std::cerr << "[MatchesMonitor]   Error broadcasting to " << player_name << ": "
                      << e.what() << std::endl;
        }
    }

    std::cout << "[MatchesMonitor] Broadcast completed: " << sent_count << " messages sent to match "
              << match_id << std::endl;
}

// ============================================
// INICIO DE PARTIDA
// ============================================

bool MatchesMonitor::start_match(int match_id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " not found\n";
        return false;
    }

    std::cout << "[MatchesMonitor] Starting match " << match_id << "\n";
    it->second->start_match();
    return true;
}

Queue<ComandMatchDTO>* MatchesMonitor::get_command_queue(int match_id) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match " << match_id << " not found\n";
        return nullptr;
    }

    return &(it->second->getComandQueue());
}

// ============================================
// ADMIN
// ============================================

void MatchesMonitor::clear_all_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    matches.clear();
    player_sockets.clear();
    player_to_match.clear();
    id_matches = 0;
}

std::string MatchesMonitor::get_match_name(int match_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = matches.find(match_id);
    return (it != matches.end()) ? it->second->get_match_name() : "";
}

// ============================================
// ALIASES DE COMPATIBILIDAD
// ============================================

void MatchesMonitor::set_player_car(int player_id, const std::string& car_name,
                                    const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& [id, match] : matches) {
        if (match->has_player(player_id)) {
            match->set_player_car(player_id, car_name, car_type);
            return;
        }
    }
}

void MatchesMonitor::delete_player_from_match(int player_id, int match_id) {
    leave_match_by_id(player_id, match_id);
}
