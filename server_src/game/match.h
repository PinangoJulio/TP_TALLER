#ifndef MATCH_H
#define MATCH_H

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../network/client_monitor.h"
#include "race.h"

class Race;

class Match {
private:
    std::string host_name;
    int match_code;
    std::atomic<bool> is_active;
    std::vector<std::unique_ptr<Race>> races;
    int current_race_index;

    ClientMonitor players_queues;
    Queue<ComandMatchDTO> command_queue;
    int max_players;
    std::vector<std::unique_ptr<Player>> players;

public:
    Match(std::string host_name, int code, int max_players);

    // ---- LOBBY ----
    bool can_player_join_match() const;
    bool add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora);
    bool remove_player(int id_jugador);
    void set_car(int player_id, const std::string& car_name, const std::string& car_type);
    void add_race(const std::string& yaml_path, const std::string& city_name);

    // ---- CARRERAS ----
    void start_next_race();
    bool is_running() const { return is_active.load(); }

    // ---- GETTERS ----
    std::string get_host_name() const { return host_name; }
    int getMatchCode() const { return match_code; }
    int get_player_count() const { return players.size(); }
    int get_max_players() const { return max_players; }
    bool is_empty() const;  

    Queue<ComandMatchDTO>& getComandQueue() { return command_queue; }

    ~Match();
};

#endif //MATCH_H