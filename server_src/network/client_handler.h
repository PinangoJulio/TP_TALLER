#ifndef SERVER_CLIENT_HANDLER_H
#define SERVER_CLIENT_HANDLER_H

#include "../../common_src/dtos.h"
#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "common_src/game_state.h"
#include "matches_monitor.h"
#include "receiver.h"
#include "sender.h"
#include "server_src/server_protocol.h"

class ClientHandler {
private:
    Socket skt;
    int client_id;
    ServerProtocol protocol;
    MatchesMonitor& monitor;

    std::atomic<bool> is_alive;
    Queue<GameState> messages_queue;  // enviadora

    Receiver receiver;

public:
    explicit ClientHandler(Socket skt, int id, MatchesMonitor& monitor);

    void run_threads();
    bool is_running();
    void stop_connection();

    Queue<GameState>& get_message_queue() { return messages_queue; }
    int get_id() const { return client_id; }

    ~ClientHandler();
};

#endif  // SERVER_CLIENT_HANDLER_H
