#include "race.h"

Race::Race(Queue<ComandMatchDTO>& cmdQueue,
           ClientMonitor& brdcstr,
           const std::string& yaml_mapa,
           Configuration& config)
    : commandQueue(cmdQueue),
      broadcaster(brdcstr),
      map_path(yaml_mapa) {
    
    std::cout << "[Race] Created race for map: " << yaml_mapa << std::endl;
    
    // Se tienen que adaptar las queues, por ahora placeholder
    // Queue<Command> temp_queue;
    // Monitor temp_monitor;
    // gameLoop = std::make_unique<GameLoop>(temp_monitor, temp_queue, config);
}