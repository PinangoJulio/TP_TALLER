#include "matches_monitor.h"

int MatchesMonitor::create_match(std::string data_yaml_path, std::string host_name, int player_id, Queue<GameState> &sender_message_queue) {
    std::lock_guard<std::mutex> lock(mtx);
    id_matches++;
    matches[id_matches] = std::make_unique<Match>(host_name, id_matches, player_id, data_yaml_path);
    matches[id_matches]->add_player(player_id, host_name, sender_message_queue);
    return id_matches;
}


