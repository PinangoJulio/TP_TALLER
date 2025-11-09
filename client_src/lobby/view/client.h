
#ifndef CLIENT_H
#define CLIENT_H
#include <string>

#include "client_protocol.h"
class Client {
private:
    ClientProtocol protocol_c;

public:
    /* Constructor del Cliente */
    explicit Client(const std::string& host, const std::string& port);

    void run();

    void read_n_messages(const int& messages);

    /* No se puede copiar clientes */
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
};


#endif  // CLIENT_H
