#ifndef SERVER_ACCEPTOR_H
#define SERVER_ACCEPTOR_H

#include <atomic>
#include <list>
#include <string>

#include "../common_src/socket.h"
#include "../common_src/thread.h"
#include "network/client_handler.h"
#include "network/matches_monitor.h"
#include "network/monitor.h"

class Acceptor : public Thread {
private:
    Socket socket;
    int client_counter;
    std::list<ClientHandler*> clients_connected;
    std::atomic<bool> is_running;

    void manage_clients_connections(MatchesMonitor& monitor);
    void clear_disconnected_clients();
    void clear_all_connections();

public:
    explicit Acceptor(const char* servicename);

    void run() override;
    void stop() override;
    void stop_accepting();

    virtual ~Acceptor();
};
#endif  // SERVER_ACCEPTOR_H
