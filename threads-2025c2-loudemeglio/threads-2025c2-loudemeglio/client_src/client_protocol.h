#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include <string>
#include <vector>

#include "../common_src/socket.h"
#include "../common_src/utils.h"

class ClientProtocol {
private:
    const std::string hostname;
    const std::string servicename;
    Socket socket;

public:
    /*
     * Constructor del protocolo por parte del cliente. Recibe hostname y servicename (o puerto)
     * para conectarse
     */
    explicit ClientProtocol(const std::string& hostname, const std::string& servicename);

    void send_msg_nitro();
    ServerMsg read_msg();

    // No se hacen copias
    ClientProtocol(const ClientProtocol&) = delete;
    ClientProtocol& operator=(const ClientProtocol&) = delete;
};
#endif  // CLIENT_PROTOCOL_H
