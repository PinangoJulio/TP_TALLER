#ifndef CLIENT_PROTOCOL_H
#define CLIENT_PROTOCOL_H
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "common_src/dtos.h"
#include "common_src/game_state.h"
#include "common_src/lobby_protocol.h"
#include "common_src/socket.h"

class ClientProtocol {
private:
    Socket socket;
    std::string host;
    std::string port;
    bool socket_shutdown_done = false;
    void push_back_uint16(std::vector<uint8_t>& message, std::uint16_t value);
    void serialize_command(const ComandMatchDTO& command, std::vector<uint8_t>& message);
    void push_back_float01_as_uint8(std::vector<uint8_t>& message, float value);


public:
    explicit ClientProtocol(const char* host, const char* servname);

    uint8_t read_message_type();
    std::string read_string();
    uint16_t read_uint16();
    uint8_t read_uint8();
    uint32_t read_uint32();
    int16_t read_int16();



    void send_string(const std::string& str);
    void send_username(const std::string& user);

    std::vector<GameInfo> read_games_list_from_socket(uint16_t count);
    std::vector<GameInfo> receive_games_list();
    void request_games_list();
    void create_game(const std::string& game_name, uint8_t max_players,
                     const std::vector<std::pair<std::string, std::string>>& races);
    uint16_t receive_game_created();
    void join_game(uint16_t game_id);
    uint16_t receive_game_joined();
    void read_error_details(std::string& error_message);

    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);
    void select_car(const std::string& car_name, const std::string& car_type);
    void start_game(uint16_t game_id);
    void leave_game(uint16_t game_id);
    void set_ready(bool is_ready);
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
    receive_city_maps();

    void send_command_client(const ComandMatchDTO& command);
    GameState receive_snapshot();
    int receive_client_id();

    // Recibir información inicial de la carrera
    RaceInfoDTO receive_race_info();

    // Utilidades
    std::string get_host() const { return host; }
    std::string get_port() const { return port; }
    void disconnect();
    void shutdown_socket();  // Desbloquear lecturas pendientes

    ClientProtocol(const ClientProtocol&) = delete;
    ClientProtocol& operator=(const ClientProtocol&) = delete;
    ClientProtocol(ClientProtocol&&) = delete;
    ClientProtocol& operator=(ClientProtocol&&) = delete;
    ~ClientProtocol();
};
#endif  // CLIENT_PROTOCOL_H
