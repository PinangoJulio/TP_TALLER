#ifndef COMMON_PROTOCOL_H
#define COMMON_PROTOCOL_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <arpa/inet.h>

#include "common_constants.h"
#include "common_socket.h"

struct UserDto {
    std::string username;

    UserDto() = default;
    explicit UserDto(const std::string& name): username(name) {}
};

struct CarDto {
    std::string name;
    uint16_t year;
    float price;

    CarDto(): name(""), year(0), price(0.0f) {}
    CarDto(const std::string& n, uint16_t y, float p): name(n), year(y), price(p) {}
};

struct MoneyDto {
    uint32_t amount;

    MoneyDto(): amount(0) {}
    explicit MoneyDto(uint32_t amt): amount(amt) {}
};

struct MarketDto {
    std::vector<CarDto> cars;

    MarketDto() = default;
    explicit MarketDto(const std::vector<CarDto>& car_list): cars(car_list) {}
};

struct CarPurchaseDto {
    CarDto car;
    uint32_t remaining_money;

    CarPurchaseDto(): remaining_money(0) {}
    CarPurchaseDto(const CarDto& c, uint32_t money): car(c), remaining_money(money) {}
};

struct ErrorDto {
    std::string message;

    ErrorDto() = default;
    explicit ErrorDto(const std::string& msg): message(msg) {}
};


class MessageBuffer {
private:
    std::vector<uint8_t> buffer;

public:
    MessageBuffer() { buffer.reserve(BUFFER_RESERVE_SIZE); }

    void clear() { buffer.clear(); }

    //////////////////////// MÉTODOS DE SERIALIZACIÓN ////////////////////////
    void append_byte(uint8_t value);
    void append_uint16(uint16_t value);
    void append_uint32(uint32_t value);
    void append_float(float value);
    void append_string(const std::string& str);
    void append_car(const CarDto& car);

    //////////////////////// ACCESO A DATOS ////////////////////////
    const uint8_t* data() const { return buffer.data(); }
    size_t size() const { return buffer.size(); }
};

class Protocol {
private:
    Socket socket;
    MessageBuffer send_buffer;
    bool connection_active;

    //////////////////////// MÉTODOS PRIVADOS DE SERIALIZACIÓN ////////////////////////
    void serialize_user(const UserDto& user);
    void serialize_money(const MoneyDto& money);
    void serialize_car(const CarDto& car);
    void serialize_market(const MarketDto& market);
    void serialize_car_purchase(const CarPurchaseDto& purchase);
    void serialize_error(const ErrorDto& error);

    //////////////////////// MÉTODOS PRIVADOS DE DESERIALIZACIÓN ////////////////////////
    UserDto deserialize_user();
    MoneyDto deserialize_money();
    CarDto deserialize_car();
    MarketDto deserialize_market();
    CarPurchaseDto deserialize_car_purchase();
    ErrorDto deserialize_error();

public:
    explicit Protocol(Socket&& skt);

    //////////////////////// INTERFAZ PÚBLICA DE ENVÍO ////////////////////////
    void send_user_registration(const UserDto& user);
    void send_initial_balance(const MoneyDto& money);
    void send_current_car_info(const CarDto& car);
    void send_market_catalog(const MarketDto& market);
    void send_purchase_confirmation(const CarPurchaseDto& purchase);
    void send_error_notification(const ErrorDto& error);

    //////////////////////// REQUESTS SIN DATOS ////////////////////////
    void send_current_car_request();
    void send_market_info_request();
    void send_car_purchase_request(const std::string& car_name);

    //////////////////////// INTERFAZ PÚBLICA DE RECEPCIÓN ////////////////////////
    uint8_t receive_command();
    UserDto receive_user_registration();
    MoneyDto receive_initial_balance();
    CarDto receive_current_car_info();
    MarketDto receive_market_catalog();
    CarPurchaseDto receive_purchase_confirmation();
    ErrorDto receive_error_notification();
    std::string receive_car_purchase_request();

    //////////////////////// MÉTODO DE ENVÍO UNIFICADO ////////////////////////
    void flush_message(uint8_t command_code);

    //////////////////////// CONTROL DE CONEXIÓN (NUEVO) ////////////////////////
    bool is_connection_active() const { return connection_active; }
    void close_connection() { connection_active = false; }

    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;
    Protocol(Protocol&&) = default;
    Protocol& operator=(Protocol&&) = default;
};

#endif  // COMMON_PROTOCOL_H
