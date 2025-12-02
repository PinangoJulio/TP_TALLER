#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <string>

#include "acceptor.h"

class Server {
private:
    Acceptor acceptor;
    std::atomic<bool> shutdown_signal; 

    void accept_connection();

public:
    explicit Server(const char* servicename);
    void start();
    void shutdown(); 
    ~Server();
};

#endif
