#include "common_protocol.h"

#include <cstring>
#include <stdexcept>
#include <utility>

//////////////////////// IMPLEMENTACIÓN DE MessageBuffer ////////////////////////

void MessageBuffer::append_byte(uint8_t value) { buffer.push_back(value); }

void MessageBuffer::append_uint16(uint16_t value) {
    uint16_t network_value = htons(value);
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&network_value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(uint16_t));
}

void MessageBuffer::append_uint32(uint32_t value) {
    uint32_t network_value = htonl(value);
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&network_value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(uint32_t));
}

void MessageBuffer::append_float(float value) {
    uint32_t price_as_int = static_cast<uint32_t>(value * PRICE_CONVERSION_FACTOR);
    append_uint32(price_as_int);
}

void MessageBuffer::append_string(const std::string& str) {
    append_uint16(str.length());
    buffer.insert(buffer.end(), str.begin(), str.end());
}

void MessageBuffer::append_car(const CarDto& car) {
    append_string(car.name);
    append_uint16(car.year);
    append_float(car.price);
}

//////////////////////// IMPLEMENTACIÓN DE Protocol ////////////////////////

Protocol::Protocol(Socket&& skt): socket(std::move(skt)), connection_active(true) {}

//////////////////////// MÉTODOS PÚBLICOS DE ENVÍO ////////////////////////

void Protocol::send_user_registration(const UserDto& user) {
    send_buffer.clear();
    serialize_user(user);
    flush_message(SEND_USERNAME);
}

void Protocol::send_initial_balance(const MoneyDto& money) {
    send_buffer.clear();
    serialize_money(money);
    flush_message(SEND_INITIAL_MONEY);
}

void Protocol::send_current_car_info(const CarDto& car) {
    send_buffer.clear();
    serialize_car(car);
    flush_message(SEND_CURRENT_CAR);
}

void Protocol::send_market_catalog(const MarketDto& market) {
    send_buffer.clear();
    serialize_market(market);
    flush_message(SEND_MARKET_INFO);
}

void Protocol::send_purchase_confirmation(const CarPurchaseDto& purchase) {
    send_buffer.clear();
    serialize_car_purchase(purchase);
    flush_message(SEND_CAR_BOUGHT);
}

void Protocol::send_error_notification(const ErrorDto& error) {
    send_buffer.clear();
    serialize_error(error);
    flush_message(SEND_ERROR_MESSAGE);
}

void Protocol::send_current_car_request() {
    send_buffer.clear();
    flush_message(GET_CURRENT_CAR);
}

void Protocol::send_market_info_request() {
    send_buffer.clear();
    flush_message(GET_MARKET_INFO);
}

void Protocol::send_car_purchase_request(const std::string& car_name) {
    send_buffer.clear();
    send_buffer.append_string(car_name);
    flush_message(BUY_CAR);
}

//////////////////////// MÉTODO DE ENVÍO UNIFICADO ////////////////////////

void Protocol::flush_message(uint8_t command_code) {
    std::vector<uint8_t> complete_message;
    complete_message.reserve(1 + send_buffer.size());
    complete_message.push_back(command_code);

    if (send_buffer.size() > 0) {
        const uint8_t* data = send_buffer.data();
        complete_message.insert(complete_message.end(), data, data + send_buffer.size());
    }

    socket.sendall(complete_message.data(), complete_message.size());
}

//////////////////////// MÉTODOS PRIVADOS DE SERIALIZACIÓN ////////////////////////

void Protocol::serialize_user(const UserDto& user) { send_buffer.append_string(user.username); }

void Protocol::serialize_money(const MoneyDto& money) { send_buffer.append_uint32(money.amount); }

void Protocol::serialize_car(const CarDto& car) { send_buffer.append_car(car); }

void Protocol::serialize_market(const MarketDto& market) {
    send_buffer.append_uint16(market.cars.size());
    for (const auto& car: market.cars) {
        send_buffer.append_car(car);
    }
}

void Protocol::serialize_car_purchase(const CarPurchaseDto& purchase) {
    send_buffer.append_car(purchase.car);
    send_buffer.append_uint32(purchase.remaining_money);
}

void Protocol::serialize_error(const ErrorDto& error) { send_buffer.append_string(error.message); }

//////////////////////// MÉTODOS PÚBLICOS DE RECEPCIÓN ////////////////////////

uint8_t Protocol::receive_command() {
    uint8_t command = 0;
    int ret = socket.recvall(&command, sizeof(command));
    if (ret == 0) {
        connection_active = false;
        throw std::runtime_error("Client disconnected");
    }
    return command;
}

UserDto Protocol::receive_user_registration() { return deserialize_user(); }

MoneyDto Protocol::receive_initial_balance() { return deserialize_money(); }

CarDto Protocol::receive_current_car_info() { return deserialize_car(); }

MarketDto Protocol::receive_market_catalog() { return deserialize_market(); }

CarPurchaseDto Protocol::receive_purchase_confirmation() { return deserialize_car_purchase(); }

ErrorDto Protocol::receive_error_notification() { return deserialize_error(); }

std::string Protocol::receive_car_purchase_request() {
    uint16_t length;
    socket.recvall(&length, sizeof(length));
    length = ntohs(length);

    std::string car_name(length, '\0');
    socket.recvall(&car_name[0], length);
    return car_name;
}

//////////////////////// MÉTODOS PRIVADOS DE DESERIALIZACIÓN ////////////////////////

UserDto Protocol::deserialize_user() {
    uint16_t length;
    socket.recvall(&length, sizeof(length));
    length = ntohs(length);

    std::string username(length, '\0');
    socket.recvall(&username[0], length);
    return UserDto(username);
}

MoneyDto Protocol::deserialize_money() {
    uint32_t amount;
    socket.recvall(&amount, sizeof(amount));
    amount = ntohl(amount);
    return MoneyDto(amount);
}

CarDto Protocol::deserialize_car() {
    uint16_t name_length;
    socket.recvall(&name_length, sizeof(name_length));
    name_length = ntohs(name_length);

    std::string name(name_length, '\0');
    socket.recvall(&name[0], name_length);

    uint16_t year;
    socket.recvall(&year, sizeof(year));
    year = ntohs(year);

    uint32_t price_as_int;
    socket.recvall(&price_as_int, sizeof(price_as_int));
    price_as_int = ntohl(price_as_int);

    float price = static_cast<float>(price_as_int) / PRICE_CONVERSION_FACTOR;

    return CarDto(name, year, price);
}

MarketDto Protocol::deserialize_market() {
    uint16_t num_cars;
    socket.recvall(&num_cars, sizeof(num_cars));
    num_cars = ntohs(num_cars);

    std::vector<CarDto> cars;
    cars.reserve(num_cars);

    for (uint16_t i = 0; i < num_cars; i++) {
        cars.push_back(deserialize_car());
    }

    return MarketDto(cars);
}

CarPurchaseDto Protocol::deserialize_car_purchase() {
    CarDto car = deserialize_car();

    uint32_t remaining_money;
    socket.recvall(&remaining_money, sizeof(remaining_money));
    remaining_money = ntohl(remaining_money);

    return CarPurchaseDto(car, remaining_money);
}

ErrorDto Protocol::deserialize_error() {
    uint16_t length;
    socket.recvall(&length, sizeof(length));
    length = ntohs(length);

    std::string message(length, '\0');
    socket.recvall(&message[0], length);
    return ErrorDto(message);
}
