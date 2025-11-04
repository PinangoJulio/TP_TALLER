#ifndef SERVER_ACCEPTOR_H
#define SERVER_ACCEPTOR_H

#include <list>
#include <string>

#include "../common_src/socket.h"
#include "../common_src/thread.h"

#include "network/client_handler.h"
#include "network/monitor.h"

class Acceptor: public Thread {
private:
    Socket socket;
    Monitor& _monitor;
    int client_counter;
    std::list<ClientHandler*> clients_connected;
    std::atomic<bool> is_running;
    Queue<struct Command>& game_queue;

    void manage_clients_connections();
    void clear_disconnected_clients();
    void clear_all_connections();

public:
    /* Acceptor: thread acepta clientes y los agrega a la lista general de todos los
     * clientes conectados Con su socket y un id asociado
     */
    explicit Acceptor(const std::string& servicename, Monitor& monitor,
                      Queue<struct Command>& queue);

    // lanzar el hilo
    void run() override;

    void stop() override;

    // No se hacen copias
    Acceptor(const Acceptor&) = delete;
    Acceptor& operator=(const Acceptor&) = delete;
};
#endif  // SERVER_ACCEPTOR_H
