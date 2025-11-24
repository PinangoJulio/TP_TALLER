#include "race.h"
#include "game_loop.h"
#include <iostream>

Race::Race(Queue<ComandMatchDTO>& cmdQueue,
           ClientMonitor& brdcstr,
           const std::string& yaml_mapa)
    : commandQueue(cmdQueue),
      broadcaster(brdcstr),
      gameLoop(nullptr),
      map_path(yaml_mapa),
      running(false),
      adapter_running(false) {
    
    std::cout << "[Race] Created race for map: " << yaml_mapa << std::endl;
}

void Race::start() {
    if (running) {
        std::cout << "[Race] Already running!" << std::endl;
        return;
    }
    
    try {
        // Crear GameLoop
        Configuracion config("config.yaml");
        Monitor monitor;
        
        // Crear queue adaptadora
        game_command_queue = std::make_unique<Queue<Command>>();
        gameLoop = std::make_unique<GameLoop>(monitor, *game_command_queue, config);
        gameLoop->load_map(map_path);

        // Iniciar thread adaptador de comandos
        adapter_running = true;
        adapter_thread = std::thread([this]() {
            std::cout << "[Race] Command adapter thread started" << std::endl;
            while (adapter_running) {
                try {
                    ComandMatchDTO match_cmd = commandQueue.pop();
                
                    Command game_cmd;
                    game_cmd.player_id = match_cmd.player_id;
                    game_cmd.action = match_cmd.command;
                
                    game_command_queue->push(game_cmd);
                
                } catch (const ClosedQueue&) {
                    std::cout << "[Race] Command queue closed" << std::endl;
                    break;
                } catch (const std::exception& e) {
                    std::cerr << "[Race] Adapter error: " << e.what() << std::endl;
                }
            }
            std::cout << "[Race] Command adapter thread stopped" << std::endl;
        });
    
        // Iniciar GameLoop
        gameLoop->start();
        running = true;
    
        std::cout << "[Race] Race started successfully" << std::endl;
    
    } catch (const std::exception& e) {
        std::cerr << "[Race] Failed to start: " << e.what() << std::endl;
        running = false;
    }
}

void Race::stop() {
    if (!running) return;
    std::cout << "[Race] Stopping race..." << std::endl;

    adapter_running = false;

    if (gameLoop) {
        gameLoop->stop();
    }

    if (game_command_queue) {
        game_command_queue->close();
    }

    running = false;

    std::cout << "[Race] Race stopped" << std::endl;
}

void Race::join() {
    if (gameLoop) {
        gameLoop->join();
        std::cout << "[Race] GameLoop joined" << std::endl;
    }
    
    if (adapter_thread.joinable()) {
        adapter_thread.join();
        std::cout << "[Race] Adapter thread joined" << std::endl;
    }

}

bool Race::isRunning() const {
    return running && gameLoop && gameLoop->is_alive();
}

Race::~Race() {
    if (running) {
        stop();
    }
    join();
}

