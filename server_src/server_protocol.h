#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H
#include <cstdint>
#include <string>
#include <vector>

#include "common_src/socket.h"
#include "lobby/lobby_manager.h"

class ServerProtocol {
    Socket& socket;
    LobbyManager& lobby_manager;


public:
    ServerProtocol(Socket& s, LobbyManager& manager);

    bool handle_client();

    // Procesa mensajes del cliente
    void process_client_messages(const std::string& username);

    // Lee un tipo de mensaje
    uint8_t read_message_type();

    // Lee una cadena
    std::string read_string();

    // Lee un uint16_t
    uint16_t read_uint16();

    // Envía un buffer
    void send_buffer(const std::vector<uint8_t>& buffer);
};

#endif //SERVER_PROTOCOL_H