#ifndef MATCHES_MONITOR_H
#define MATCHES_MONITOR_H

#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include <string>

#include "common_src/game_state.h"
#include "common_src/lobby_protocol.h"
#include "common_src/queue.h"
#include "server_src/game/match.h"

class MatchesMonitor {
private:
    int id_matches = 0;
    std::mutex mtx;
    std::map<int, std::unique_ptr<Match>> matches;

public:
    int create_match(int max_players, const std::string& host_name, int player_id, Queue<GameState>& sender_message_queue);
    bool add_races_to_match(int match_id, const std::vector<RaceConfig>& race_paths);
    std::vector<GameInfo> list_available_matches();
    bool join_match(int match_id, const std::string& player_name, int player_id, Queue<GameState>& sender_message_queue);

    bool is_player_in_game(const std::string& username);
    void set_player_car(int player_id, const std::string& car_name, const std::string& car_type);
    void delete_player_from_match(int player_id, int match_id);
    void clear_all_matches();

    void start_match(int match_id); // inicia la lógica del juego
};

#endif // MATCHES_MONITOR_H
