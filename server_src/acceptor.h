#ifndef SERVER_ACCEPTOR_H
#define SERVER_ACCEPTOR_H

#include <list>
#include <string>

#include "../common_src/socket.h"
#include "../common_src/thread.h"

#include "network/client_handler.h"
#include "network/monitor.h"
#include "network/matches_monitor.h"

class Acceptor: public Thread {
private:
    Socket socket;
    //Monitor& _monitor;
    int client_counter;
    std::list<ClientHandler*> clients_connected;
    std::atomic<bool> is_running;
    //Queue<struct Command>& game_queue;
    LobbyManager lobby_manager;

    void manage_clients_connections(MatchesMonitor& monitor);
    void clear_disconnected_clients();
    void clear_all_connections();

public:
    /* Acceptor: thread acepta clientes y los agrega a la lista general de todos los
     * clientes conectados Con su socket y un id asociado
     */
    explicit Acceptor(const char *servicename);

    // lanzar el hilo
    void run() override;

    void stop() override;
    void stop_accepting();


     virtual ~Acceptor();
};
#endif  // SERVER_ACCEPTOR_H
