#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <string>
#include <vector>

#include "client_src/lobby/view/common_types.h"
#include "common_src/socket.h"
#include "common_src/lobby_protocol.h"

class LobbyClient {
private:
    Socket socket;
    std::string username;
    bool connected;

    uint8_t read_message_type();
    std::string read_string();
    uint16_t read_uint16();
    uint8_t read_uint8();
    void send_string(const std::string& str);

public:
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
    void create_game(const std::string &game_name, uint8_t max_players, const std::vector<std::pair<std::string, std::string>> &races);

    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> receive_city_maps();

    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);

    void select_car(const std::string& car_name, const std::string& car_type);

    std::string receive_car_confirmation();

    // Recibir confirmación de juego creado
    uint16_t receive_game_created();

    // Unirse a un juego
    void join_game(uint16_t game_id);

    // Recibir confirmación de unión a juego
    uint16_t receive_game_joined();

    // Lee el tipo de mensaje (para decisiones en el cliente)
    uint8_t peek_message_type();

    // Lee detalles de error (asume que MSG_ERROR ya fue leído)
    void read_error_details(std::string& error_message);

    void start_game(uint16_t game_id);


//===================================================================================================================
    // COSAS QUE NECESITO PARA TERMINAR LA INTEGRACIÓN DEL LOBBY
    // En las vistas siguen siendo place holders
    // void leave_game(uint16_t game_id);
    // void select_car(uint8_t car_index);
    // void set_ready(bool is_ready);
    // void start_game(uint16_t game_id);
//===================================================================================================================
   
    // Getters
    bool is_connected() const { return connected; }
    std::string get_username() const { return username; }

    // No se pueden copiar
    LobbyClient(const LobbyClient&) = delete;
    LobbyClient& operator=(const LobbyClient&) = delete;
};

#endif // LOBBY_CLIENT_H