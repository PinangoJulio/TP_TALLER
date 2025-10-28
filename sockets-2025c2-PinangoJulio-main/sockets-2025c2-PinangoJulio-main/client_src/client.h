#ifndef CLIENT_H
#define CLIENT_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../common_src/common_constants.h"
#include "../common_src/common_protocol.h"
#include "../common_src/common_socket.h"

class Client {
private:
    Socket socket;
    Protocol protocol;

    std::map<std::string, std::function<void(const std::string&)>> command_handlers;

    //////////////////////// CARGA Y EJECUCIÓN DE LOS COMANDOS ////////////////////////
    void setup_command_handlers();
    void load_and_execute_commands(const std::string& filename);
    void process_username_command(std::ifstream& file);
    void receive_and_display_initial_balance();
    void execute_remaining_commands(std::ifstream& file);
    void execute_command(const std::string& command, const std::string& parameter);

    //////////////////////// REQUESTS AL SERVIDOR ////////////////////////
    void request_current_car();
    void request_market_info();
    void request_buy_car(const std::string& car_name);

    //////////////////////// MANEJO DE RESPUESTAS DEL SERVIDOR ////////////////////////
    void handle_current_car_response();
    void handle_market_info_response();
    void handle_buy_car_response();

    //////////////////////// UTILIDADES DE DISPLAY ////////////////////////
    void print_car_info(const CarDto& car, const std::string& prefix = "");
    void print_market_info(const std::vector<CarDto>& cars);
    void print_purchase_confirmation(const CarPurchaseDto& purchase);
    void print_error_message(const std::string& error_msg);

public:
    Client(const std::string& hostname, const std::string& port, const std::string& commands_file);

    void run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = default;
    Client& operator=(Client&&) = default;
};

#endif  // CLIENT_H
