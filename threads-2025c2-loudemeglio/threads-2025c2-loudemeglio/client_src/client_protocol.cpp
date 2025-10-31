#include "client_protocol.h"

#include <stdexcept>

#include <netinet/in.h>


ClientProtocol::ClientProtocol(const std::string& hostname, const std::string& servicename):
        hostname(hostname),
        servicename(servicename),
        socket(hostname.c_str(), servicename.c_str()) {}

void ClientProtocol::send_msg_nitro() {
    uint8_t msg = CodeActions::MSG_CLIENT;
    int bytes = this->socket.sendall(&msg, sizeof(msg));

    if (bytes <= 0) {
        throw std::runtime_error("Failed to send 'nitro' message from Client");
    }
}

// Formato del Mensaje recibido:
// -->  [ 0x10 ][<cars-with-nitro>][<nitro-activated|nitro-expired>]

ServerMsg ClientProtocol::read_msg() {
    ServerMsg msg;
    this->socket.recvall(&msg, sizeof(msg));

    // cars_with_nitro en big endian -> convertir a host order
    ServerMsg msg_received;
    msg_received.type = msg.type;
    msg_received.cars_with_nitro = ntohs(msg.cars_with_nitro);
    msg_received.nitro_status = msg.nitro_status;

    return msg_received;
}
