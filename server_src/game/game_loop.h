#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"  

#include "car.h"
#include "../network/monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250

class GameLoop: public Thread {
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
    explicit GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue);

    void run() override;
    void stop() override { is_running = false; }

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
};

#endif  // SERVER_GAME_H
