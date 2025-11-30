#ifndef RACE_H
#define RACE_H

#include <atomic>
#include <iostream>
#include <memory>
#include <string>

#include "../../common_src/queue.h"
#include "../network/client_monitor.h"
#include "game_loop.h" 

class Race {
private:
    std::unique_ptr<GameLoop> gameLoop;
    Queue<ComandMatchDTO>& commandQueue;
    ClientMonitor& broadcaster;
    std::string city_name;
    std::string map_yaml_path;

public:
    Race(Queue<ComandMatchDTO>& cmdQueue, ClientMonitor& brdcstr, const std::string& city,
         const std::string& yaml_mapa)
        : commandQueue(cmdQueue), broadcaster(brdcstr), city_name(city), map_yaml_path(yaml_mapa) {
        
        gameLoop = std::make_unique<GameLoop>(commandQueue, broadcaster, map_yaml_path);
        
        gameLoop->set_city_name(city);
    }

    void start() {
        std::cout << "[Race] Iniciando carrera en " << city_name << " (mapa: " << map_yaml_path
                  << ")\n";
        
        gameLoop->start(); 
    }

    void stop() {
        std::cout << "[Race] Deteniendo carrera en " << city_name << "\n";
        if (gameLoop) {
            gameLoop->stop();
            gameLoop->join();
        }
    }


    void add_player_to_gameloop(int player_id, const std::string& name, const std::string& car_name,
                                const std::string& car_type) {
        gameLoop->add_player(player_id, name, car_name, car_type);
    }

    void set_player_ready(int player_id, bool ready) {
        gameLoop->set_player_ready(player_id, ready);
    }

    void set_total_laps(int laps) { gameLoop->set_total_laps(laps); }
    void set_city(const std::string& city) { 
        city_name = city;
        gameLoop->set_city_name(city); 
    }

    const std::string& get_city_name() const { return city_name; }
    const std::string& get_map_path() const { return map_yaml_path; }

    ~Race() {
        if (gameLoop && gameLoop->is_alive()) {
            gameLoop->stop();
            gameLoop->join();
        }
    }
};

#endif
