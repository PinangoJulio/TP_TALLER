#include "server_protocol.h"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

ServerProtocol::ServerProtocol(Socket &skt, LobbyManager& manager)
    : socket(skt), lobby_manager(manager) {}

// --- Lectura básica de datos ---

uint8_t ServerProtocol::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0)
        throw std::runtime_error("Connection closed");
    return type;
}

std::string ServerProtocol::read_string() {
    uint16_t len_net;
    int bytes_read = socket.recvall(&len_net, sizeof(len_net));
    if (bytes_read == 0)
        throw std::runtime_error("Connection closed while reading string length");

    uint16_t len = ntohs(len_net);
    if (len == 0 || len > 1024)
        throw std::runtime_error("Invalid string length: " + std::to_string(len));

    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    return std::string(buffer.begin(), buffer.end());
}

uint16_t ServerProtocol::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void ServerProtocol::send_buffer(const std::vector<uint8_t>& buffer) {
    socket.sendall(buffer.data(), buffer.size());
}


uint8_t ServerProtocol::get_uint8_t() {
    uint8_t n;
    socket.recvall(&n, sizeof(n));
    return n;
}