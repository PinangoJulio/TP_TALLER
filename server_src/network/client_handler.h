#ifndef SERVER_CLIENT_HANDLER_H
#define SERVER_CLIENT_HANDLER_H
#include "../common_src/queue.h"
#include "../common_src/socket.h"

#include "server_car.h"
#include "monitor.h"
#include "../game/game_protocol.h"
#include "receiver.h"
#include "sender.h"

/*  Cada ClientHandler maneja la conexion de un cliente
 *  Con su socket, y los threads Receiver y Sender para
 *  recibir/enviar mensajes a los clientes correspondientes
 */

class ClientHandler {
private:
    Socket skt;
    const int client_id;
    ServerProtocol protocol_s;
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


    // No se permiten copias
    ClientHandler(const ClientHandler&) = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;
};

#endif  // SERVER_CLIENT_HANDLER_H
