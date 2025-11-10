#ifndef SENDER_H
#define SENDER_H

#include "../../common_src/queue.h"
#include "../server_protocol.h"
#include "../../common_src/thread.h"
#include "../../common_src/game_state.h"

class Sender : public Thread {
private:
    ServerProtocol& protocol;
    Queue<GameState>& sender_queue;
    std::atomic<bool>& alive;
    int player_id;

public:
    Sender(ServerProtocol& protocol, Queue<GameState>& sender_queue,
           std::atomic<bool>& alive, int player_id);

    void run() override ;

    ~Sender();
};

#endif
