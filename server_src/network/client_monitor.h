#ifndef CLIENT_MONITOR_H
#define CLIENT_MONITOR_H
#include <list>
#include <mutex>
#include <utility>

#include "common_src/game_state.h"
#include "common_src/queue.h"

class ClientMonitor {
    std::list<std::pair<Queue<GameState>&, int>> queues_list;  // recurso compartido
    std::mutex mtx;

public:
    ClientMonitor();

    // Add new client
    void add_client_queue(Queue<GameState>& queue, int player_id);

    // recieve a particular status of the game and it is added to every client queue
    void broadcast(const GameState& state);

    void delete_client_queue(int player_id);
};

#endif  // CLIENT_MONITOR_H
