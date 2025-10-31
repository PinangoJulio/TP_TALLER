
#ifndef SERVER_RECEIVER_H
#define SERVER_RECEIVER_H

#include "../common_src/queue.h"
#include "../common_src/socket.h"
#include "../common_src/thread.h"

#include "server_monitor.h"
#include "server_protocol.h"

/* Thread Receiver
 *  Espera mensajes por socket, los deserializa y manda hacia la queue del juego
 */
class Receiver: public Thread {
    ServerProtocol& protocol_s;
    std::atomic<bool> is_alive;
    const int id;
    Queue<struct Command>& game_queue;

public:
    explicit Receiver(ServerProtocol& protocol_s, const int id, Queue<struct Command>& queue);

    /* Mienras el hilo este activo recibe los comandos y los manda a la queue del gameloop */
    void run() override;

    void kill();

    bool status();

    // No se hacen copias
    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;
};

#endif  // SERVER_RECEIVER_H
