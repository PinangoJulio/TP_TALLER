#ifndef LOBBY_SERVER_H
#define LOBBY_SERVER_H

#include <string>
#include <thread>
#include <cstring>
#include "../../common_src/socket.h"
#include "../../common_src/dtos.h"
#include "../../common_src/lobby_protocol.h"
#include "lobby_manager.h"

class LobbyServer {
private:
    Socket acceptor_socket;
    LobbyManager lobby_manager;
    bool running;

    // Maneja la conexión de un cliente
    void handle_client(Socket client_socket);
    
    // Recibe el username del cliente
    std::string receive_username(Socket& client_socket);
    
    // Envía mensaje de bienvenida
    void send_welcome(Socket& client_socket, const std::string& username);
    
    // Envía la lista de partidas
    void send_games_list(Socket& client_socket);
    
    // Envía confirmación de partida creada
    void send_game_created(Socket& client_socket, uint16_t game_id);
    
    // Envía confirmación de unión a partida
    void send_game_joined(Socket& client_socket, uint16_t game_id);
    
    // Envía un mensaje de error
    void send_error(Socket& client_socket, LobbyErrorCode error_code, const std::string& message);
    
    // Lee un tipo de mensaje
    uint8_t read_message_type(Socket& client_socket);
    
    // Lee una cadena
    std::string read_string(Socket& client_socket);
    
    // Lee un uint16_t
    uint16_t read_uint16(Socket& client_socket);

public:
    // Constructor
    explicit LobbyServer(const std::string& port);

    // Ejecuta el servidor
    void run();

    // Detiene el servidor
    void stop();

    // Destructor
    ~LobbyServer();

    // No se puede copiar
    LobbyServer(const LobbyServer&) = delete;
    LobbyServer& operator=(const LobbyServer&) = delete;
};

#endif // LOBBY_SERVER_H
