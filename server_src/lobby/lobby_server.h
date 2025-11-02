
#ifndef LOBBY_SERVER_H
#define LOBBY_SERVER_H

#include <string>
#include "../../common_src/socket.h"
#include "lobby_manager.h"

class LobbyServer {
private:
    Socket acceptor_socket;
    LobbyManager lobby_manager;
    bool running;

    // Maneja la conexión de un cliente
    void handle_client(Socket client_socket);
    
    // Procesa mensajes del cliente
    void process_client_messages(Socket& client_socket, const std::string& username);
    
    // Lee un tipo de mensaje
    uint8_t read_message_type(Socket& socket);
    
    // Lee una cadena
    std::string read_string(Socket& socket);
    
    // Lee un uint16_t
    uint16_t read_uint16(Socket& socket);
    
    // Envía un buffer
    void send_buffer(Socket& socket, const std::vector<uint8_t>& buffer);

public:
    // Constructor
    explicit LobbyServer(const std::string& port);

    // Ejecuta el servidor
    void run();

    // Detiene el servidor
    void stop();

    // No se puede copiar
    LobbyServer(const LobbyServer&) = delete;
    LobbyServer& operator=(const LobbyServer&) = delete;
};

#endif // LOBBY_SERVER_H
