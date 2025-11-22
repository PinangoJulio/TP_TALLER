#include "client_handler.h"
#include <utility>

ClientHandler::ClientHandler(Socket skt, int id, MatchesMonitor& monitor):
skt(std::move(skt)),
client_id(id),
protocol(this->skt),
monitor(monitor),
is_alive(true),
messages_queue(),
receiver(protocol, this->client_id, messages_queue, is_alive, monitor) {}

void ClientHandler::run_threads() {
    receiver.start();
}

void ClientHandler::stop_connection() {
    is_alive = false;
}

bool ClientHandler::is_running() {
    return is_alive;
}

ClientHandler::~ClientHandler() {
    stop_connection();
    receiver.kill();
    receiver.join();
}