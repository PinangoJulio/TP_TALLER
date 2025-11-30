#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H

#include <string>
#include <vector>
#include <cstdint>

#include "common_src/dtos.h"
#include "common_src/socket.h"
#include "common_src/game_state.h"

class ClientProtocol {
private:
    Socket socket;
    std::string host;
    std::string port;
    bool socket_shutdown_done = false;

    // --- NUEVO: Buffer para devolver bytes ---
    bool has_buffered_byte = false;
    uint8_t buffered_byte = 0;

public:
    ClientProtocol(const char* host, const char* servname);

    // Métodos existentes...
    void shutdown_socket();
    void disconnect();

    // Métodos de lectura/escritura básicos
    uint8_t read_message_type();
    
    // --- NUEVO: Método para devolver el byte ---
    void unread_message_type(uint8_t byte);

    std::string read_string();
    uint16_t read_uint16();
    uint8_t read_uint8();
    void send_string(const std::string& str);

    // Lobby
    void send_username(const std::string& user);
    void request_games_list();
    std::vector<GameInfo> read_games_list_from_socket(uint16_t count);
    std::vector<GameInfo> receive_games_list();
    void create_game(const std::string& game_name, uint8_t max_players,
                     const std::vector<std::pair<std::string, std::string>>& races);
    uint16_t receive_game_created();
    void join_game(uint16_t game_id);
    uint16_t receive_game_joined();
    void read_error_details(std::string& error_message);
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> receive_city_maps();
    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);
    void select_car(const std::string& car_name, const std::string& car_type);
    void start_game(uint16_t game_id);
    void leave_game(uint16_t game_id);
    void set_ready(bool is_ready);

    // Game
    void send_command_client(const ComandMatchDTO& command);
    void serialize_command(const ComandMatchDTO& command, std::vector<uint8_t>& message);
    void push_back_uint16(std::vector<uint8_t>& message, std::uint16_t value);
    GameState receive_snapshot();
    int receive_client_id();
    RaceInfoDTO receive_race_info(); // Asegúrate de que este exista

    ~ClientProtocol();
};

#endif
