#include "server_client_handler.h"

#include <utility>


ClientHandler::ClientHandler(Socket&& skt, const int id, Queue<struct Command>& queue):
        skt(std::move(skt)),
        client_id(id),
        protocol_s(this->skt),
        messages_queue(),
        receiver(this->protocol_s, this->client_id, queue),
        sender(protocol_s, messages_queue),
        game_queue(queue) {}

void ClientHandler::run_threads() {
    receiver.start();
    sender.start();
}

void ClientHandler::stop_threads() {
    receiver.kill();
    sender.kill();
}

void ClientHandler::join_threads() {
    receiver.join();
    sender.join();
}


bool ClientHandler::is_alive() { return receiver.status() || sender.status(); }
