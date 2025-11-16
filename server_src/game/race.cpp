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
      running(false) {
    
    std::cout << "[Race] Created race for map: " << yaml_mapa << std::endl;
    
    // TODO: Cuando GameLoop esté adaptado para usar ComandMatchDTO:
    // Configuracion config("config.yaml");
    // Monitor monitor;  // Crear un monitor temporal
    // Queue<Command> temp_queue;  // Convertir ComandMatchDTO a Command
    // gameLoop = std::make_unique<GameLoop>(monitor, temp_queue, config);
}

void Race::start() {
    if (gameLoop) {
        gameLoop->start();
        running = true;
        std::cout << "[Race] Race started" << std::endl;
    } else {
        std::cout << "[Race] GameLoop not initialized, cannot start" << std::endl;
    }
}

void Race::stop() {
    if (gameLoop) {
        gameLoop->stop();
        running = false;
        std::cout << "[Race] Race stopped" << std::endl;
    }
}

void Race::join() {
    if (gameLoop) {
        gameLoop->join();
        std::cout << "[Race] Race joined (thread finished)" << std::endl;
    }
}

bool Race::isRunning() const {
    return running && gameLoop && gameLoop->is_alive();
}

Race::~Race() {
    if (running) {
        stop();
    }
    if (gameLoop) {
        join();
    }
}
