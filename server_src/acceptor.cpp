#include "acceptor.h"

#include <string>
#include <utility>

#include <sys/socket.h>

Acceptor::Acceptor(const char *servicename):
        socket(servicename),
        client_counter(0),
        clients_connected(),
        is_running(true),
        lobby_manager(){}

void Acceptor::manage_clients_connections(MatchesMonitor& monitor) {

    Socket client_socket = socket.accept();
    ClientHandler* new_client = new ClientHandler(std::move(client_socket), ++client_counter, monitor, lobby_manager);
    new_client->run_threads();
    std::cout << "[LobbyServer] New connection accepted" << std::endl;
    clients_connected.push_back(new_client);

}

// iteramos por la lista de clientes y eliminamos aquellas que ya esten desconectados
void Acceptor::clear_disconnected_clients() {
    for (auto it = clients_connected.begin(); it != clients_connected.end();) {
        if (!(*it)->is_running()) {
            (*it)->stop_connection();
            delete *it;
            it = clients_connected.erase(it);
        } else {
            ++it;
        }
    }
}

void Acceptor::clear_all_connections() {
    for (auto* ch: clients_connected) {
        ch->stop_connection();
        delete ch;
    }
    clients_connected.clear();
}

void Acceptor::stop_accepting() { is_running = false; }

void Acceptor::run() {
    MatchesMonitor monitor;
    try {
        while (is_running) {
            manage_clients_connections(monitor);
            clear_disconnected_clients();
        }
    } catch (const std::exception& e) {
        if (is_running) {
            std::cerr << "Error ocurred accepting clients " << e.what() << std::endl;
        }
        is_running = false;
    }
    //monitor.clear_all_matches();
    clear_all_connections();
}

void Acceptor::stop() {
    is_running = false;
    socket.shutdown(SHUT_RDWR);
    socket.close();
    std::cout << "[LobbyServer] Server stopped" << std::endl;
}

Acceptor::~Acceptor() {
    for (auto& ch: clients_connected) {
        ch->stop_connection();
        delete ch;
    }
    clients_connected.clear();
    stop();
    this->join();
}