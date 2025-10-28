#include "server_protocol.h"

#include <iostream>
#include <ostream>
#include <string>
#include <utility>

#include <netinet/in.h>

ServerProtocol::ServerProtocol(Socket& client_skt): client_skt(std::move(client_skt)) {}

uint8_t ServerProtocol::recv_msg() {
    uint8_t msg;
    int bytes = this->client_skt.recvall(&msg, sizeof(uint8_t));
    if (bytes <= 0) {
        return 0;
    }
    return msg;
}


void ServerProtocol::send_msg(ServerMsg& msg) {
    ServerMsg server_msg;
    server_msg.type = msg.type;
    server_msg.cars_with_nitro = htons(msg.cars_with_nitro);
    server_msg.nitro_status = msg.nitro_status;

    this->client_skt.sendall(&server_msg, sizeof(ServerMsg));
}
