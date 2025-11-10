#ifndef RACE_H
#define RACE_H
#include <atomic>
#include "game_loop.h" // Ahora Race usa el GameLoop
#include "../../common_src/queue.h"
#include "../network/client_monitor.h"
// ... (otras inclusiones) ...

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

    // MÉTODOS DE CONTROL
    //void start() { gameLoop->start(); } // Llama a Thread::start()
    //void stop() { gameLoop->stop(); }   // Llama a GameLoop::stop()
    //void join() { gameLoop->join(); }   // Llama a Thread::join()

    //bool isRunning() const { return gameLoop->isRunning(); }

    ~Race() = default;
};
#endif //RACE_H