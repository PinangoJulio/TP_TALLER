#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include <box2d/box2d.h>

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"
#include "../../common_src/config.h" 

#include "car.h"
#include "../network/monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250

class GameLoop: public Thread {
private:
    bool is_running;
    Monitor& monitor;
    std::vector<Car> cars;
    int cars_with_nitro;
    Queue<struct Command>& game_queue;
    
    b2WorldId mundo;
    Configuration& config;
    std::map<int, b2BodyId> player_bodies;
    
    void initialize_physics();
    void update_physics();
    void apply_forces_from_command(const Command& cmd, b2BodyId body);
    
    // Métodos existentes
    void send_nitro_on();
    void send_nitro_off();
    void process_commands();
    void simulate_cars();

public:
    explicit GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue, Configuration& cfg);

    void run() override;
    void stop() override { is_running = false; }
    
    ~GameLoop();

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
};

#endif