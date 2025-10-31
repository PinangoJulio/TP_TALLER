// server_game.h
#ifndef SERVER_GAME_H
#define SERVER_GAME_H
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../common_src/thread.h"
#include "../common_src/utils.h"

#include "server_car.h"
#include "server_monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250


class GameSimulator: public Thread {
    bool is_running;
    Monitor& monitor;       // para el broadcast
    std::vector<Car> cars;  // autos de cada cliente < Car, id >
    int cars_with_nitro;
    Queue<struct Command>& game_queue;

    void send_nitro_on(Car& car);

    void send_nitro_off(Car& car);

    void process_commands();

    void simulate_cars();

public:
    explicit GameSimulator(Monitor& monitor_ref, Queue<struct Command>& queue);

    void run() override;

    void stop() override { is_running = false; }

    // No se hacen copias
    GameSimulator(const GameSimulator&) = delete;
    GameSimulator& operator=(const GameSimulator&) = delete;
};

#endif  // SERVER_GAME_H
