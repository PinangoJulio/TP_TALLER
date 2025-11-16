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

class Race;

class Match {
private:
    int match_code;
    std::string host_name;
    std::atomic<bool> is_active;
    Queue<ComandMatchDTO> queue_comandos;
    ClientMonitor players_queues;
    
    int cant_actual_players;
    int cant_max_players;
    std::string race_path;
    
    std::unique_ptr<Race> active_races;

public:
    Match(std::string host_name,
          int code,
          int max_players,
          const std::string& map_yaml_path);

    bool can_player_join_match() const;
    bool add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora);
    bool remove_player(int id_jugador);

    void startNextRace(const std::string& mapa_yaml_path);
    bool isRunning() const;

    std::string get_host_name() const { return host_name; }
    int getMatchCode() const { return match_code; }
    Queue<ComandMatchDTO>& getComandQueue() { return queue_comandos; }

    ~Match();
};

#endif
