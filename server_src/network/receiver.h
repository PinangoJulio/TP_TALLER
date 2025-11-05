#ifndef SERVER_RECEIVER_H
#define SERVER_RECEIVER_H

#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"  // ✅ AGREGAR

/* Thread Receiver
 *  Espera mensajes por socket, los deserializa y manda hacia la queue del juego
 */
class Receiver: public Thread {
    Socket& socket;  // ✅ CAMBIAR: ServerProtocol → Socket
    std::atomic<bool> is_alive;
    const int id;
    Queue<struct Command>& game_queue;

public:
    explicit Receiver(Socket& socket, const int id, Queue<struct Command>& queue);

    void run() override;
    void kill();
    bool status();

    Receiver(const Receiver&) = delete;
    Receiver& operator=(const Receiver&) = delete;
};

#endif  // SERVER_RECEIVER_H
