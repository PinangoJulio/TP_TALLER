#include "sender.h"
#include <iostream>

Sender::Sender(Socket& socket, Queue<ServerMsg>& queue):
        socket(socket), mesgs_queue(queue), is_alive(true) {}

void Sender::run() {
    try {
        while (is_alive) {
            ServerMsg msg = mesgs_queue.pop();  // bloqueante
            socket.sendall(&msg, sizeof(msg));
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
