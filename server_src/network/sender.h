#ifndef SERVER_SENDER_H
#define SERVER_SENDER_H
#include "../common_src/queue.h"
#include "../common_src/thread.h"
#include "../common_src/utils.h"

#include "../game/game_protocol.h"

/*
 * Thread Sender
 * 'espera' mensajes a enviar, le llegan a traves de una Queue
 * Cada mensaje : lo serializa (protocolo) y lo envia por socket
 */
class Sender: public Thread {
    ServerProtocol& protocol;
    Queue<struct ServerMsg>& mesgs_queue;
    std::atomic<bool> is_alive;

public:
    Sender(ServerProtocol& protocol, Queue<struct ServerMsg>& mesg_queue);

    void run() override;

    void kill();

    bool status();

    // No se hacen copias
    Sender(const Sender&) = delete;
    Sender& operator=(const Sender&) = delete;
};


#endif  // SERVER_SENDER_H
