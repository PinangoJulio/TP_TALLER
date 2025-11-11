#ifndef RACE_H
#define RACE_H
#include <atomic>
#include <memory>
#include <string>

#include "game_loop.h" 
#include "../../common_src/queue.h"
#include "../network/client_monitor.h"
#include "../../common_src/config.h"  

class Race {
private:
    // Instancia directa del GameLoop (que es el hilo)
    std::unique_ptr<GameLoop> gameLoop;

    // Referencias a los recursos (para crear el GameLoop)
    Queue<ComandMatchDTO>& commandQueue;
    ClientMonitor& broadcaster;
    std::string map_path;

public:
    Race(Queue<ComandMatchDTO>& cmdQueue,
         ClientMonitor& brdcstr,
         const std::string& yaml_mapa);

    Race(Queue<ComandMatchDTO>& cmdQueue,
         ClientMonitor& brdcstr,
         const std::string& yaml_mapa,
         Configuracion& config);

    void start() { 
        if (gameLoop) gameLoop->start(); 
    }
    
    void stop() { 
        if (gameLoop) gameLoop->stop(); 
    }
    
    void join() { 
        if (gameLoop) gameLoop->join(); 
    }
    
    bool isRunning() const { 
       
    ~Race() = default;
};
#endif //RACE_H