#ifndef SERVER_CLIENT_HANDLER_H
#define SERVER_CLIENT_HANDLER_H

#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "../../common_src/dtos.h"

#include "receiver.h"
#include "sender.h"

class ClientHandler {
private:
    Socket skt;
    const int client_id;
    Queue<ServerMsg> messages_queue;
    Receiver receiver;
    Sender sender;
    Queue<struct Command>& game_queue;

public:
    explicit ClientHandler(Socket&& skt, const int id, Queue<struct Command>& queue);

    void run_threads();
    void join_threads();
    void stop_threads();
    bool is_alive();

    Queue<ServerMsg>& get_message_queue() { return messages_queue; }
    int get_id() const { return client_id; }

    ClientHandler(const ClientHandler&) = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;
};

#endif  // SERVER_CLIENT_HANDLER_H
