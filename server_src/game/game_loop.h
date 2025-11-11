#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <map>

#include <box2d/box2d.h>  // ✅ TU BOX2D

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"
#include "../../common_src/config.h"  // ✅ TU CONFIG

#include "car.h"
#include "../network/monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250

// ✅ MANTENER nombre GameLoop (de main) pero con tu contenido Box2D
class GameLoop: public Thread {
private:
    bool is_running;
    Monitor& monitor;
    std::vector<Car> cars;
    int cars_with_nitro;
    Queue<struct Command>& game_queue;
    
    // ✅ TU BOX2D
    b2WorldId mundo;
    Configuracion& config;
    std::map<int, b2BodyId> player_bodies;
    
    // ✅ TU BOX2D: Métodos de física
    void initialize_physics();
    void update_physics();
    void apply_forces_from_command(const Command& cmd, b2BodyId body);
    
    // Métodos existentes
    void send_nitro_on();
    void send_nitro_off();
    void process_commands();
    void simulate_cars();

public:
    // ✅ TU CONSTRUCTOR (con config)
    explicit GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue, Configuracion& cfg);

    void run() override;
    void stop() override { is_running = false; }
    
    // ✅ TU DESTRUCTOR
    ~GameLoop();

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
};

#endif  // SERVER_GAME_H