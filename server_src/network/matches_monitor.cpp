#include "matches_monitor.h"
#include <iostream>

int MatchesMonitor::create_match(std::string data_yaml_path, 
                                  std::string host_name, 
                                  int max_players,
                                  Queue<GameState>& sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);
    id_matches++;
    
    matches[id_matches] = std::make_unique<Match>(
        host_name, 
        id_matches, 
        max_players, 
        data_yaml_path
    );
    
    matches[id_matches]->add_player(0, host_name, sender_message_queue);
    
    std::cout << "[MatchesMonitor] Match " << id_matches << " created by " << host_name << std::endl;
    return id_matches;
}

std::vector<std::string> MatchesMonitor::list_available_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    std::vector<std::string> result;
    
    for (const auto& pair : matches) {
        result.push_back("Match " + std::to_string(pair.first) + " (Host: " + pair.second->get_host_name() + ")");
    }
    
    return result;
}

bool MatchesMonitor::join_match(int match_id, 
                                 std::string& player_name, 
                                 int player_id, 
                                 Queue<GameState>& sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = matches.find(match_id);
    if (it == matches.end()) {
        std::cout << "[MatchesMonitor] Match " << match_id << " not found" << std::endl;
        return false;
    }
    
    bool success = it->second->add_player(player_id, player_name, sender_message_queue);
    if (success) {
        std::cout << "[MatchesMonitor] Player " << player_name << " joined match " << match_id << std::endl;
    }
    return success;
}

void MatchesMonitor::clear_all_matches() {
    std::lock_guard<std::mutex> lock(mtx);
    matches.clear();
    std::cout << "[MatchesMonitor] All matches cleared" << std::endl;
}