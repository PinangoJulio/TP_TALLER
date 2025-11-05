#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"  // ✅ CAMBIAR: utils.h → dtos.h

#include "car.h"
#include "../network/monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250

class GameSimulator: public Thread {
    bool is_running;
    Monitor& monitor;
    std::vector<Car> cars;
    int cars_with_nitro;
    Queue<struct Command>& game_queue;

    void send_nitro_on();
    void send_nitro_off();
    void process_commands();
    void simulate_cars();

public:
    explicit GameSimulator(Monitor& monitor_ref, Queue<struct Command>& queue);

    void run() override;
    void stop() override { is_running = false; }

    GameSimulator(const GameSimulator&) = delete;
    GameSimulator& operator=(const GameSimulator&) = delete;
};

#endif  // SERVER_GAME_H
