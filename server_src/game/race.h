#ifndef RACE_H
#define RACE_H
#include <atomic>
#include <memory>
#include <string>

#include "../../common_src/queue.h"
#include "../network/client_monitor.h"
#include "../../common_src/dtos.h"

// Forward declaration (no incluir game_loop.h para evitar dependencias circulares)
class GameLoop;

class Race {
private:
    Queue<ComandMatchDTO>& commandQueue;
    ClientMonitor& broadcaster;
    std::unique_ptr<GameLoop> gameLoop;
    std::string map_path;
    std::atomic<bool> running;

public:
    Race(Queue<ComandMatchDTO>& cmdQueue,
         ClientMonitor& brdcstr,
         const std::string& yaml_mapa);

    void start();
    void stop();
    void join();
    bool isRunning() const;
    
    ~Race();
};
#endif
