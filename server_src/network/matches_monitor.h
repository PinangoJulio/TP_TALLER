#ifndef MATCHES_MONITOR_H
#define MATCHES_MONITOR_H
#include <map>
#include <mutex>
#include <memory>

#include "common_src/game_state.h"
#include "common_src/queue.h"
#include "server_src/game/match.h"

class Match;

class MatchesMonitor {
private:
    int id_matches = 0;
    std::mutex mtx;
    std::map<int, std::unique_ptr<Match>> matches;

public:
    MatchesMonitor() = default;
    
    int create_match(std::string data_yaml_path, 
                     std::string host_name, 
                     int max_players,
                     Queue<GameState>& sender_message_queue);
    
    std::vector<std::string> list_available_matches();
    
    bool join_match(int match_id, 
                    std::string& player_name, 
                    int player_id, 
                    Queue<GameState>& sender_message_queue);
    
    void clear_all_matches();

    ~MatchesMonitor() = default;
};

#endif
