#include "server_game.h"

GameSimulator::GameSimulator(Monitor& monitor_ref, Queue<struct Command>& queue):
        is_running(true), monitor(monitor_ref), cars_with_nitro(0), game_queue(queue) {}

void GameSimulator::send_nitro_on() {
    cars_with_nitro++;
    ServerMsg msg;
    msg.type = CodeActions::MSG_SERVER;
    msg.cars_with_nitro = (uint16_t)this->cars_with_nitro;
    msg.nitro_status = CodeActions::NITRO_ON;
    std::cout << "A car hit the nitro!" << std::endl;
    monitor.broadcast(msg);
}


void GameSimulator::send_nitro_off() {
    cars_with_nitro--;
    ServerMsg msg;
    msg.type = CodeActions::MSG_SERVER;
    msg.cars_with_nitro = (uint16_t)this->cars_with_nitro;
    msg.nitro_status = CodeActions::NITRO_OFF;
    std::cout << "A car is out of juice." << std::endl;
    monitor.broadcast(msg);
}

void GameSimulator::process_commands() {
    Command cmd;
    while (game_queue.try_pop(cmd)) {
        auto it = std::find_if(cars.begin(), cars.end(),
                               [&](const Car& c) { return c.get_client_id() == cmd.id; });

        if (it == cars.end()) {
            cars.emplace_back(cmd.id, NITRO_DURATION);
            it = cars.end() - 1;
        }

        if (cmd.action == CodeActions::NITRO_ON) {
            if (!it->is_nitro_active()) {
                it->activate_nitro();
                send_nitro_on();
            }
        }
    }
}

void GameSimulator::simulate_cars() {
    for (auto& car: cars) {
        if (car.simulate_tick()) {
            send_nitro_off();
        }
    }
}


void GameSimulator::run() {
    while (is_running) {
        // 1. Procesar todos los comandos pendientes y ejecutarlos
        process_commands();
        // 2. Simular consumicion de nitro
        simulate_cars();
        // 3. Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
}
