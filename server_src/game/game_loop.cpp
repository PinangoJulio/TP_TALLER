#include "game_loop.h"

GameSimulator::GameSimulator(Monitor& monitor_ref, Queue<struct Command>& queue):
        is_running(true), monitor(monitor_ref), cars_with_nitro(0), game_queue(queue) {}

void GameSimulator::send_nitro_on() {
    cars_with_nitro++;
    ServerMsg msg;
    msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);  // ✅ CORREGIDO
    msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_ON);  // ✅ CORREGIDO
    std::cout << "A car hit the nitro!" << std::endl;
    monitor.broadcast(msg);
}

void GameSimulator::send_nitro_off() {
    cars_with_nitro--;
    ServerMsg msg;
    msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);  // ✅ CORREGIDO
    msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_OFF);  // ✅ CORREGIDO
    std::cout << "A car is out of juice." << std::endl;
    monitor.broadcast(msg);
}

void GameSimulator::process_commands() {
    Command cmd;
    while (game_queue.try_pop(cmd)) {
        auto it = std::find_if(cars.begin(), cars.end(),
                               [&](const Car& c) { 
                                   return c.get_client_id() == cmd.player_id;  // ✅ CORREGIDO: id → player_id
                               });

        if (it == cars.end()) {
            cars.emplace_back(cmd.player_id, NITRO_DURATION);  // ✅ CORREGIDO: id → player_id
            it = cars.end() - 1;
        }

        if (cmd.action == GameCommand::USE_NITRO) {  // ✅ CORREGIDO: CodeActions::NITRO_ON → GameCommand::USE_NITRO
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
        // 2. Simular consumición de nitro
        simulate_cars();
        // 3. Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
}