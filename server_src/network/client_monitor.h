#ifndef CLIENT_MONITOR_H
#define CLIENT_MONITOR_H
#include <list>

#include "common_src/game_state.h"
#include "common_src/queue.h"


class ClientMonitor {
    std::list<std::pair<Queue<GameState>&, int>> queues_list;
    std::mutex mtx;
    public:
    ClientMonitor();

    void add_client_queue(Queue<GameState> &queue, int player_id);

    void broadcast(const GameState &state);

    void delete_client_queue(int player_id);
};


#endif