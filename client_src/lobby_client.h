#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <string>
#include <vector>
#include "../common_src/socket.h"
#include "../common_src/lobby_protocol.h"

class LobbyClient {
private:
    Socket socket;
    std::string username;
    bool connected;

    // Lee un tipo de mensaje
    uint8_t read_message_type();
    
    // Lee una cadena de texto del socket
    std::string read_string();
    
    // Lee un uint16_t del socket
    uint16_t read_uint16();

public:
    // Constructor
    LobbyClient(const std::string& host, const std::string& port);

    // Enviar username al servidor
    void send_username(const std::string& username);

    // Recibir mensaje de bienvenida
    std::string receive_welcome();

    // Pedir lista de juegos
    void request_games_list();

    // Recibir lista de juegos
    std::vector<GameInfo> receive_games_list();

    // Crear un nuevo juego
    void create_game(const std::string& game_name, uint8_t max_players);

    // Recibir confirmación de juego creado
    uint16_t receive_game_created();

    // Unirse a un juego
    void join_game(uint16_t game_id);

    // Recibir confirmación de unión a juego
    uint16_t receive_game_joined();

    // Verificar si hay error
    bool check_for_error(std::string& error_message);

    // Getters
    bool is_connected() const { return connected; }
    std::string get_username() const { return username; }

    // No se pueden copiar
    LobbyClient(const LobbyClient&) = delete;
    LobbyClient& operator=(const LobbyClient&) = delete;
};

#endif // LOBBY_CLIENT_H
