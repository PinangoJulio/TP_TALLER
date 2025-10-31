#include "server_receiver.h"

Receiver::Receiver(ServerProtocol& protocol_s, const int id, Queue<struct Command>& queue):
        protocol_s(protocol_s), is_alive(true), id(id), game_queue(queue) {}

void Receiver::run() {

    try {
        while (is_alive) {
            uint8_t raw_msg = this->protocol_s.recv_msg();  // bloqueante
            if (raw_msg == CodeActions::MSG_CLIENT) {
                Command command;
                command.action = CodeActions::NITRO_ON;
                command.id = this->id;
                game_queue.try_push(command);
            } else {
                is_alive = false;
            }
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
