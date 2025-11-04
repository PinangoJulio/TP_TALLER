#include "acceptor.h"

#include <string>
#include <utility>

#include <sys/socket.h>

Acceptor::Acceptor(const std::string& servicename, Monitor& monitor, Queue<struct Command>& queue):
        socket(servicename.c_str()),
        _monitor(monitor),
        client_counter(0),
        clients_connected(),
        is_running(true),
        game_queue(queue) {}

void Acceptor::manage_clients_connections() {
    Socket client_socket = this->socket.accept();
    // uso el heap
    ClientHandler* new_client =
            new ClientHandler(std::move(client_socket), ++client_counter, game_queue);

    _monitor.add_client_to_queue(new_client->get_message_queue());
    new_client->run_threads();
    clients_connected.push_back(new_client);
}

// iteramos por la lista de clientes y eliminamos aquellas que ya esten desconectados
void Acceptor::clear_disconnected_clients() {
    for (auto it = clients_connected.begin(); it != clients_connected.end();) {
        if (!(*it)->is_alive()) {
            (*it)->stop_threads();
            (*it)->join_threads();
            delete *it;
            it = clients_connected.erase(it);
        } else {
            ++it;
        }
    }
}

void Acceptor::clear_all_connections() {
    for (auto* ch: clients_connected) {
        ch->stop_threads();
        ch->join_threads();
        delete ch;
    }
    clients_connected.clear();
}

void Acceptor::run() {
    try {
        while (is_running) {
            manage_clients_connections();
            clear_disconnected_clients();
        }
    } catch (const std::exception& e) {
        if (is_running) {
            std::cerr << "Error ocurred accepting clients " << e.what() << std::endl;
        }
        is_running = false;
    }
    clear_all_connections();
}

void Acceptor::stop() {
    is_running = false;
    socket.shutdown(SHUT_RDWR);
    socket.close();
}
