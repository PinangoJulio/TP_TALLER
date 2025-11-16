#ifndef SERVER_H
#define SERVER_H

#include <string>
#include "acceptor.h"

class Server {
private:
    LobbyManager lobby_manager;
    Acceptor acceptor;

    void accept_connection();

public:
    /* Constructor del Server */
    explicit Server(const char *servicename);

    void start();

    ~Server(); //destructor

};

#endif  // SERVER_H
