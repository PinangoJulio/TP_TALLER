#include "receiver.h"
#include <iostream>

Receiver::Receiver(Socket& socket, const int id, Queue<struct Command>& queue):
        socket(socket), is_alive(true), id(id), game_queue(queue) {}

void Receiver::run() {
    try {
        while (is_alive) {
            uint8_t raw_msg;
            int bytes = socket.recvall(&raw_msg, sizeof(raw_msg));
            
            if (bytes == 0) {
                is_alive = false;
                break;
            }
            
            Command command;
            command.action = static_cast<GameCommand>(raw_msg);
            command.player_id = this->id;
            game_queue.try_push(command);
        }
    } catch (const std::exception& e) {
        if (is_alive) {
            std::cerr << "Error in thread receiver: " << e.what() << std::endl;
        }
        is_alive = false;
    }
}

void Receiver::kill() { is_alive = false; }

bool Receiver::status() { return is_alive; }