#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H
#include <string>

#include "../common_src/socket.h"
#include "../common_src/utils.h"


class ServerProtocol {
private:
    Socket client_skt;

    Socket accept_client();

public:
    explicit ServerProtocol(Socket& client_skt);

    void initialize_socket();

    uint8_t recv_msg();

    void send_msg(ServerMsg& msg);

    ServerProtocol(const ServerProtocol&) = delete;
    ServerProtocol& operator=(const ServerProtocol&) = delete;
};
#endif  // SERVER_PROTOCOL_H
