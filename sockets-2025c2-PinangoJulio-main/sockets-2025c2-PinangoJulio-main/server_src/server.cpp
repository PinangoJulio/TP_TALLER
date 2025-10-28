#include "server.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

//////////////////////// Constructor ////////////////////////

Server::Server(const std::string& port, const std::string& market_file):
        acceptor_socket(port.c_str()), initial_money(0), client_money(0) {
    load_market_data(market_file);
    client_money = initial_money;
    std::cout << "Server started" << std::endl;
}

//////////////////////// INICIALIZACIÓN DEL SERVIDOR ////////////////////////

void Server::load_market_data(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error al abrir el archivo: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            parse_line(line);
        }
    }

    if (initial_money == 0) {
        throw std::runtime_error("NO hay Money en el archivo");
    }
}

void Server::parse_line(const std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command;

    if (command == "money") {
        parse_money_config(iss);
    } else if (command == "car") {
        parse_car_config(iss);
    }
}

void Server::parse_money_config(std::istringstream& iss) {
    uint32_t money_value;
    iss >> money_value;
    initial_money = money_value;
}

void Server::parse_car_config(std::istringstream& iss) {
    std::string name;
    uint16_t year;
    float price;
    iss >> name >> year >> price;
    market_cars.emplace_back(name, year, price);
}

//////////////////////// MANEJO DE CONEXIÓN CON CLIENTE ////////////////////////

void Server::run() { handle_client_connection(); }

void Server::handle_client_connection() {
    Socket client_socket = acceptor_socket.accept();
    Protocol protocol(std::move(client_socket));

    try {
        process_initial_handshake(protocol);
        process_client_commands(protocol);
    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        if (error_msg.find("Client disconnected") != std::string::npos) {
            std::cerr << "Conexion del Server terminado: Se desconectó el Cliente" << std::endl;
        } else {
            std::cerr << "Conexion del Server terminado: " << e.what() << std::endl;
        }
    }
}

void Server::process_initial_handshake(Protocol& protocol) {
    uint8_t first_command = protocol.receive_command();

    if (first_command != SEND_USERNAME) {
        throw std::runtime_error("Debe usarse el username como Primer Mensaje");
    }

    UserDto user = protocol.receive_user_registration();
    client_username = user.username;
    log_client_welcome();

    MoneyDto initial_balance(initial_money);
    protocol.send_initial_balance(initial_balance);
    log_initial_balance_sent();

    client_money = initial_money;
}

void Server::process_client_commands(Protocol& protocol) {
    while (protocol.is_connection_active()) {
        try {
            uint8_t command = protocol.receive_command();

            switch (command) {
                case GET_CURRENT_CAR:
                    handle_current_car_request(protocol);
                    break;
                case GET_MARKET_INFO:
                    handle_market_info_request(protocol);
                    break;
                case BUY_CAR:
                    handle_car_purchase_request(protocol);
                    break;
                default:
                    std::cerr << "Comando desconcido recivido: 0x" << std::hex << (int)command
                              << std::dec << std::endl;
                    protocol.close_connection();
                    break;
            }
        } catch (const std::runtime_error& e) {
            std::string error_msg = e.what();
            if (error_msg.find("Client disconnected") != std::string::npos) {
                break;
            }
            throw;
        }
    }
}

//////////////////////// HANDLERS DE COMANDOS ESPECÍFICOS ////////////////////////

void Server::handle_current_car_request(Protocol& protocol) {
    if (client_current_car.has_value()) {
        send_current_car_info(protocol);
        log_current_car_sent(client_current_car.value());
    } else {
        send_no_car_error(protocol);
        log_error("No car bought");
    }
}

void Server::handle_market_info_request(Protocol& protocol) {
    send_market_catalog(protocol);
    log_market_sent();
}

void Server::handle_car_purchase_request(Protocol& protocol) {
    std::string car_name = protocol.receive_car_purchase_request();

    const CarDto* car = find_car_by_name(car_name);
    if (car == nullptr) {
        send_car_not_found_error(protocol);
        log_error("Car not found");
        return;
    }

    if (!has_sufficient_funds(*car)) {
        send_insufficient_funds_error(protocol);
        log_error("Insufficient funds");
        return;
    }

    process_successful_purchase(*car);
    send_purchase_confirmation(protocol, *car);
    log_successful_purchase(*car);
}

//////////////////////// LÓGICA DE NEGOCIO ////////////////////////

const CarDto* Server::find_car_by_name(const std::string& name) const {
    auto it = std::find_if(market_cars.begin(), market_cars.end(),
                           [&name](const CarDto& car) { return car.name == name; });

    return (it != market_cars.end()) ? &(*it) : nullptr;
}

bool Server::has_sufficient_funds(const CarDto& car) const {
    return client_money >= static_cast<uint32_t>(car.price);
}

void Server::process_successful_purchase(const CarDto& car) {
    client_money -= static_cast<uint32_t>(car.price);
    client_current_car = car;
}

//////////////////////// UTILIDADES DE RESPUESTA ////////////////////////

void Server::send_current_car_info(Protocol& protocol) {
    protocol.send_current_car_info(client_current_car.value());
}

void Server::send_no_car_error(Protocol& protocol) {
    ErrorDto error("No car bought");
    protocol.send_error_notification(error);
}

void Server::send_market_catalog(Protocol& protocol) {
    MarketDto market(market_cars);
    protocol.send_market_catalog(market);
}

void Server::send_purchase_confirmation(Protocol& protocol, const CarDto& car) {
    CarPurchaseDto purchase(car, client_money);
    protocol.send_purchase_confirmation(purchase);
}

void Server::send_insufficient_funds_error(Protocol& protocol) {
    ErrorDto error("Insufficient funds");
    protocol.send_error_notification(error);
}

void Server::send_car_not_found_error(Protocol& protocol) {
    ErrorDto error("Car not found");
    protocol.send_error_notification(error);
}

//////////////////////// LOGGING ////////////////////////

void Server::log_client_welcome() { std::cout << "Hello, " << client_username << std::endl; }

void Server::log_initial_balance_sent() {
    std::cout << "Initial balance: " << initial_money << std::endl;
}

void Server::log_current_car_sent(const CarDto& car) {
    std::cout << "Car " << car.name << " " << static_cast<uint32_t>(car.price) << " " << car.year
              << " sent" << std::endl;
}

void Server::log_market_sent() { std::cout << market_cars.size() << " cars sent" << std::endl; }

void Server::log_successful_purchase(const CarDto& car) {
    std::cout << "New cars name: " << car.name << " --- remaining balance: " << client_money
              << std::endl;
}

void Server::log_error(const std::string& error_msg) {
    std::cout << "Error: " << error_msg << std::endl;
}
