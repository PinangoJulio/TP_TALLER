#include "matches_monitor.h"

#include <cstring>


int MatchesMonitor::create_match(int max_players, const std::string &host_name, int player_id, Queue<GameState> &sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);

    int match_id = ++id_matches;

    // El path inicial del mapa se puede dejar vacío, se agrega luego con add_races_to_match()
    std::unique_ptr<Match> match = std::make_unique<Match>(
        host_name,
        match_id,
        max_players
    );

    // Agregamos el host como primer jugador
    match->add_player(player_id, host_name, sender_message_queue);
    matches.emplace(match_id, std::move(match));

    std::cout << "[MatchesMonitor] Partida creada: ID=" << match_id
              << " Host=" << host_name << std::endl;

    return match_id;
}


bool MatchesMonitor::add_races_to_match(int match_id, const std::vector<RaceConfig>& races) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] Match no encontrado: " << match_id << std::endl;
        return false;
    }

    for (const auto& race_cfg : races) {
        std::string yaml_path = "city_maps/" + race_cfg.city + "/" + race_cfg.map + ".yaml";
        it->second->add_race(yaml_path, race_cfg.city);
    }

    std::cout << "[MatchesMonitor] Carreras agregadas a match " << match_id << std::endl;
    return true;
}

bool MatchesMonitor::join_match(int match_id, const std::string &player_name, int player_id, Queue<GameState> &sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cerr << "[MatchesMonitor] No existe el match con id " << match_id << std::endl;
        return false;
    }

    auto& match = it->second;

    if (!match->can_player_join_match()) {
        std::cerr << "[MatchesMonitor] El match " << match_id << " está lleno." << std::endl;
        return false;
    }

    return match->add_player(player_id, player_name, sender_message_queue);
}

std::vector<GameInfo> MatchesMonitor::list_available_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<GameInfo> result;

    for (auto& [id, match] : matches) {
        GameInfo info{};
        info.game_id = id;
        strncpy(info.game_name, match->get_host_name().c_str(), sizeof(info.game_name));
        info.current_players = match->get_player_count();
        info.max_players = match->get_max_players();
        info.is_started = match->is_running();
        result.push_back(info);
    }

    return result;
}

void MatchesMonitor::set_player_car(int player_id, const std::string& car_name, const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);
    for (auto& [id, match] : matches) {
        match->set_car(player_id, car_name, car_type);
    }
}

void MatchesMonitor::delete_player_from_match(int player_id, int match_id) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = matches.find(match_id);
    if (it != matches.end()) {
        it->second->remove_player(player_id);
    }
}

void MatchesMonitor::clear_all_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    matches.clear();
    id_matches = 0;
}

