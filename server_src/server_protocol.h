#ifndef SERVER_PROTOCOL_H
#define SERVER_PROTOCOL_H
#include <cstdint>
#include <string>
#include <vector>

#include "common_src/dtos.h"
#include "common_src/game_state.h"
#include "common_src/socket.h"

class ServerProtocol {
    Socket& socket;

public:
    explicit ServerProtocol(Socket& s);

    // Procesa mensajes del cliente
    bool process_client_messages(const std::string& username);

    // Lee un tipo de mensaje
    uint8_t read_message_type();

    // Lee una cadena
    std::string read_string();

    // Lee un uint16_t
    uint16_t read_uint16();

    // Lee un uint8_t
    uint8_t get_uint8_t();
    float read_float32();


    // Envía un buffer
    void send_buffer(const std::vector<uint8_t>& buffer);

    //envia client id
    bool send_client_id(int client_id);

    // Obtener referencia al socket
    Socket& get_socket() { return socket; }

    // leer comando cliente
    bool read_command_client(ComandMatchDTO& command);

    // Enviar snapshot (GameState) al cliente
    bool send_snapshot(const GameState& snapshot);

    // Enviar información inicial de la carrera
    bool send_race_info(const RaceInfoDTO& race_info);

    // Enviar rutas YAML de las carreras de la partida
    bool send_race_paths(const std::vector<std::string>& yaml_paths);
};

#endif  // SERVER_PROTOCOL_H
