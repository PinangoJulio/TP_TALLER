#include "acceptor.h"

#include <string>
#include <utility>

#include <sys/socket.h>

Acceptor::Acceptor(const char *servicename):
        socket(servicename),
        client_counter(0),
        clients_connected(),
        is_running(true),
        lobby_manager(){
    std::cout << "[Acceptor] Socket created on port " << servicename << std::endl;
}

void Acceptor::manage_clients_connections(MatchesMonitor& monitor) {
    Socket client_socket = socket.accept();
    ClientHandler* new_client = new ClientHandler(std::move(client_socket), ++client_counter, monitor, lobby_manager);
    new_client->run_threads();
    
    // 🔥 LOGS MEJORADOS
    std::cout << "==================================================" << std::endl;
    std::cout << "[Acceptor] New connection accepted" << std::endl;
    std::cout << "           Client ID: " << client_counter << std::endl;
    std::cout << "           Total clients: " << (clients_connected.size() + 1) << std::endl;
    std::cout << "==================================================" << std::endl;
    
    clients_connected.push_back(new_client);
}

// iteramos por la lista de clientes y eliminamos aquellas que ya esten desconectados
void Acceptor::clear_disconnected_clients() {
    size_t before = clients_connected.size();
    
    for (auto it = clients_connected.begin(); it != clients_connected.end();) {
        if (!(*it)->is_running()) {
            int client_id = (*it)->get_id();
            (*it)->stop_connection();
            delete *it;
            it = clients_connected.erase(it);
            
            // 🔥 LOG de desconexión
            std::cout << "[Acceptor] Client " << client_id << " disconnected and cleaned up" << std::endl;
        } else {
            ++it;
        }
    }
    
    size_t after = clients_connected.size();
    if (before != after) {
        std::cout << "[Acceptor] Active clients: " << after << std::endl;
    }
}

void Acceptor::clear_all_connections() {
    std::cout << "[Acceptor] Clearing all " << clients_connected.size() << " connections..." << std::endl;
    
    for (auto* ch: clients_connected) {
        ch->stop_connection();
        delete ch;
    }
    clients_connected.clear();
    
    std::cout << "[Acceptor] All connections closed" << std::endl;
}

void Acceptor::stop_accepting() { 
    is_running = false; 
    std::cout << "[Acceptor] Stop signal received" << std::endl;
}

void Acceptor::run() {
    MatchesMonitor monitor;
    try {
        while (is_running) {
            manage_clients_connections(monitor);
            clear_disconnected_clients();
        }
    } catch (const std::exception& e) {
        if (is_running) {
            std::cerr << "[Acceptor] Error occurred: " << e.what() << std::endl;
        }
        is_running = false;
    }
    
    clear_all_connections();
    std::cout << "[Acceptor] Thread stopped" << std::endl;
}

void Acceptor::stop() {
    is_running = false;
    socket.shutdown(SHUT_RDWR);
    socket.close();
    std::cout << "[Acceptor] Socket closed" << std::endl;
}

Acceptor::~Acceptor() {
    for (auto& ch: clients_connected) {
        ch->stop_connection();
        delete ch;
    }
    clients_connected.clear();
    stop();
    this->join();
    std::cout << "[Acceptor] Destructor completed" << std::endl;
}