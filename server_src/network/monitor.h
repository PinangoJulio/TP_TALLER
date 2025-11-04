
#ifndef SERVER_MONITOR_H
#define SERVER_MONITOR_H
#include <list>
#include <mutex>
#include <string>

#include "../common_src/queue.h"
#include "../common_src/utils.h"

class Monitor {
private:
    std::mutex mtx;  // proteger el acceso a la lista de colas de mensajes a Clientes
    std::list<Queue<struct ServerMsg>*> message_queue;

public:
    Monitor();

    void broadcast(const ServerMsg& server_msg);
    void add_client_to_queue(Queue<struct ServerMsg>& client_queue);

    void clean_queue();

    // No se hacen copias
    Monitor(const Monitor&) = delete;
    Monitor& operator=(const Monitor&) = delete;
};


#endif  // SERVER_MONITOR_H
