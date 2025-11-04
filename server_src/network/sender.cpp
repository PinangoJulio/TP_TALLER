#include "sender.h"

#include "../game/game_protocol.h"

Sender::Sender(ServerProtocol& protocol, Queue<ServerMsg>& queue):
        protocol(protocol), mesgs_queue(queue), is_alive(true) {}


void Sender::run() {
    try {
        while (is_alive) {
            ServerMsg msg = mesgs_queue.pop();  // bloqueante
            protocol.send_msg(msg);
        }
    } catch (const std::exception& e) {
        if (is_alive) {
            std::cerr << "Error while trying to send messages!" << std::endl;
        }
        is_alive = false;
    }
}

void Sender::kill() {
    is_alive = false;
    mesgs_queue.close();
}


bool Sender::status() { return is_alive; }
