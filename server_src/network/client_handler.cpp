#include "client_handler.h"
#include <utility>

ClientHandler::ClientHandler(Socket skt, int id, MatchesMonitor& monitor, LobbyManager& lobby_manager):
skt(std::move(skt)),
client_id(id),
protocol(this->skt, lobby_manager),
monitor(monitor),
is_alive(true),
messages_queue(),
lobby_manager(lobby_manager),
receiver(protocol, this->client_id, messages_queue, is_alive, monitor, lobby_manager) {}

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