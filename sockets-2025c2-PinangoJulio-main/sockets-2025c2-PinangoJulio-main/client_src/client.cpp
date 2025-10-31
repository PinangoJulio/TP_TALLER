#include "client.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

Client::Client(const std::string& hostname, const std::string& port,
               const std::string& commands_file):
        socket(hostname.c_str(), port.c_str()), protocol(std::move(socket)) {
    setup_command_handlers();
    load_and_execute_commands(commands_file);
}

//////////////////////// CARGA Y EJECUCIÓN DE COMANDOS ////////////////////////

void Client::setup_command_handlers() {
    command_handlers["get_current_car"] = [this](const std::string&) { request_current_car(); };
    command_handlers["get_market"] = [this](const std::string&) { request_market_info(); };
    command_handlers["buy_car"] = [this](const std::string& param) { request_buy_car(param); };
}

void Client::load_and_execute_commands(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error en abrir el archivo: " + filename);
    }
    process_username_command(file);
    receive_and_display_initial_balance();
    execute_remaining_commands(file);
}

void Client::process_username_command(std::ifstream& file) {
    std::string line;
    bool username_found = false;

    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "username") {
                std::string username_param;
                iss >> username_param;
                UserDto user(username_param);
                protocol.send_user_registration(user);
                username_found = true;
                break;
            }
        }
    }

    if (!username_found) {
        throw std::runtime_error("NO existe ningun comando para username en el file");
    }
}

void Client::receive_and_display_initial_balance() {
    uint8_t response = protocol.receive_command();
    if (response != SEND_INITIAL_MONEY) {
        throw std::runtime_error("Se esperaba: initial money de server");
    }

    MoneyDto initial_money = protocol.receive_initial_balance();
    std::cout << "Initial balance: " << initial_money.amount << std::endl;
}

void Client::execute_remaining_commands(std::ifstream& file) {
    file.clear();
    file.seekg(0);

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            std::istringstream iss(line);
            std::string command;
            iss >> command;

            if (command == "username") {
                continue;
            }

            std::string parameter;
            std::getline(iss, parameter);
            parameter.erase(0, parameter.find_first_not_of(" \t"));

            execute_command(command, parameter);
        }
    }
}

void Client::execute_command(const std::string& command, const std::string& parameter) {
    auto it = command_handlers.find(command);
    if (it != command_handlers.end()) {
        it->second(parameter);
    } else {
        std::cerr << "Comando Desconocido: " << command << std::endl;
    }
}

//////////////////////// REQUESTS AL SERVIDOR ////////////////////////

void Client::request_current_car() {
    protocol.send_current_car_request();
    handle_current_car_response();
}

void Client::request_market_info() {
    protocol.send_market_info_request();
    handle_market_info_response();
}

void Client::request_buy_car(const std::string& car_name) {
    protocol.send_car_purchase_request(car_name);
    handle_buy_car_response();
}

//////////////////////// MANEJO DE RESPUESTAS DEL SERVIDOR ////////////////////////

void Client::handle_current_car_response() {
    uint8_t command = protocol.receive_command();

    if (command == SEND_CURRENT_CAR) {
        CarDto current_car = protocol.receive_current_car_info();
        print_car_info(current_car, "Current car: ");
    } else if (command == SEND_ERROR_MESSAGE) {
        ErrorDto error = protocol.receive_error_notification();
        print_error_message(error.message);
    } else {
        throw std::runtime_error("Unexpected response from server");
    }
}

void Client::handle_market_info_response() {
    uint8_t command = protocol.receive_command();
    if (command != SEND_MARKET_INFO) {
        throw std::runtime_error("Expected market info from server");
    }

    MarketDto market = protocol.receive_market_catalog();
    print_market_info(market.cars);
}

void Client::handle_buy_car_response() {
    uint8_t command = protocol.receive_command();

    if (command == SEND_CAR_BOUGHT) {
        CarPurchaseDto purchase = protocol.receive_purchase_confirmation();
        print_purchase_confirmation(purchase);
    } else if (command == SEND_ERROR_MESSAGE) {
        ErrorDto error = protocol.receive_error_notification();
        print_error_message(error.message);
    } else {
        throw std::runtime_error("Unexpected response from server");
    }
}

//////////////////////// UTILIDADES DE DISPLAY ////////////////////////

void Client::print_car_info(const CarDto& car, const std::string& prefix) {
    std::cout << prefix << car.name << ", year: " << car.year << ", price: " << std::fixed
              << std::setprecision(2) << car.price << std::endl;
}

void Client::print_market_info(const std::vector<CarDto>& cars) {
    for (const auto& car: cars) {
        std::cout << car.name << ", year: " << car.year << ", price: " << std::fixed
                  << std::setprecision(2) << car.price << std::endl;
    }
}

void Client::print_purchase_confirmation(const CarPurchaseDto& purchase) {
    std::cout << "Car bought: " << purchase.car.name << ", year: " << purchase.car.year
              << ", price: " << std::fixed << std::setprecision(2) << purchase.car.price
              << std::endl;
    std::cout << "Remaining balance: " << purchase.remaining_money << std::endl;
}

void Client::print_error_message(const std::string& error_msg) {
    std::cout << "Error: " << error_msg << std::endl;
}

void Client::run() {}
