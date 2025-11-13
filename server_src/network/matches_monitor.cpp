#include "matches_monitor.h"

int MatchesMonitor::create_match(std::string data_yaml_path, std::string host_name, int player_id, Queue<GameState> &sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);
    id_matches++;
    matches[id_matches] = std::make_unique<Match>(host_name, id_matches, player_id, data_yaml_path);
    matches[id_matches]->add_player(player_id, host_name, sender_message_queue);
    return id_matches;
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


