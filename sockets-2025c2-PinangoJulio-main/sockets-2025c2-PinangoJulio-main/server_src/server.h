#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "../common_src/common_constants.h"
#include "../common_src/common_protocol.h"
#include "../common_src/common_socket.h"

class Server {
private:
    Socket acceptor_socket;
    std::vector<CarDto> market_cars;
    uint32_t initial_money;

    std::string client_username;
    uint32_t client_money;
    std::optional<CarDto> client_current_car;

    //////////////////////// INICIALIZACIÓN DEL SERVIDOR ////////////////////////
    void load_market_data(const std::string& filename);
    void parse_line(const std::string& line);
    void parse_money_config(std::istringstream& iss);
    void parse_car_config(std::istringstream& iss);

    //////////////////////// MANEJO DE CONEXIÓN CON CLIENTE ////////////////////////
    void handle_client_connection();
    void process_initial_handshake(Protocol& protocol);
    void process_client_commands(Protocol& protocol);

    //////////////////////// HANDLERS DE COMANDOS ESPECÍFICOS ////////////////////////
    void handle_current_car_request(Protocol& protocol);
    void handle_market_info_request(Protocol& protocol);
    void handle_car_purchase_request(Protocol& protocol);

    //////////////////////// LÓGICA DE NEGOCIO ////////////////////////
    const CarDto* find_car_by_name(const std::string& name) const;
    bool has_sufficient_funds(const CarDto& car) const;
    void process_successful_purchase(const CarDto& car);

    //////////////////////// UTILIDADES DE RESPUESTA ////////////////////////
    void send_current_car_info(Protocol& protocol);
    void send_no_car_error(Protocol& protocol);
    void send_market_catalog(Protocol& protocol);
    void send_purchase_confirmation(Protocol& protocol, const CarDto& car);
    void send_insufficient_funds_error(Protocol& protocol);
    void send_car_not_found_error(Protocol& protocol);

    //////////////////////// LOGGING ////////////////////////
    void log_client_welcome();
    void log_initial_balance_sent();
    void log_current_car_sent(const CarDto& car);
    void log_market_sent();
    void log_successful_purchase(const CarDto& car);
    void log_error(const std::string& error_msg);

public:
    explicit Server(const std::string& port, const std::string& market_file);

    void run();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;
    Server(Server&&) = default;
    Server& operator=(Server&&) = default;
};

#endif  // SERVER_H
