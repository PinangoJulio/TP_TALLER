// server_src/acceptor.cpp

#include "acceptor.h"
#include <sys/socket.h>
#include <string>
#include <utility>
#include <thread>
#include <chrono>
#include <arpa/inet.h>

Acceptor::Acceptor(const char* servicename)
    : socket(servicename), 
      client_counter(0), 
      clients_connected(), 
      is_running(true),
      is_accepting(false)
{
    std::cout << "[Acceptor] Socket created on port " << servicename << std::endl;
}

void Acceptor::notify_shutdown_to_all_clients() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    std::cout << "[Acceptor] Notifying " << clients_connected.size()
              << " clients about shutdown..." << std::endl;
    
    // Crear mensaje de shutdown
    std::vector<uint8_t> shutdown_msg;
    shutdown_msg.push_back(0xFF); // MSG_ERROR
    shutdown_msg.push_back(0xFF); // Código especial
    std::string msg = "SERVER SHUTDOWN - DISCONNECTING";
    uint16_t len = htons(msg.size());
    shutdown_msg.push_back(reinterpret_cast<uint8_t*>(&len)[0]);
    shutdown_msg.push_back(reinterpret_cast<uint8_t*>(&len)[1]);
    shutdown_msg.insert(shutdown_msg.end(), msg.begin(), msg.end());
    
    // Enviar a todos
    for (auto* client : clients_connected) {
        if (client) {
            try {
                client->send_shutdown_message(shutdown_msg);
            } catch (const std::exception& e) {
                std::cerr << "[Acceptor] Error notifying client " 
                          << client->get_id() << ": " << e.what() << std::endl;
            }
        }
    }
    
    std::cout << "[Acceptor] Shutdown notifications sent" << std::endl;

    // Esto previene que los receivers procesen DISCONNECT después del shutdown
    std::cout << "[Acceptor] Stopping all client connections..." << std::endl;
    for (auto* client : clients_connected) {
        if (client) {
            try {
                client->stop_connection();
            } catch (const std::exception& e) {
                std::cerr << "[Acceptor] Error stopping client "
                          << client->get_id() << ": " << e.what() << std::endl;
            }
        }
    }
    std::cout << "[Acceptor]All connections stopped" << std::endl;
}

void Acceptor::close_socket() {
    try {
        socket.shutdown(SHUT_RDWR);
        socket.close();
        std::cout << "[Acceptor]  Socket closed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Acceptor] Error closing socket: " << e.what() << std::endl;
    }
}

void Acceptor::manage_clients_connections(MatchesMonitor& monitor) {
    Socket client_socket = socket.accept();
    ClientHandler* new_client = new ClientHandler(std::move(client_socket), ++client_counter, monitor);
    new_client->run_threads();
    
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients_connected.push_back(new_client);
    
    std::cout << "==================================================" << std::endl;
    std::cout << "[Acceptor] New connection accepted" << std::endl;
    std::cout << "           Client ID: " << client_counter << std::endl;
    std::cout << "           Total clients: " << clients_connected.size() << std::endl;
    std::cout << "==================================================" << std::endl;
}

void Acceptor::clear_disconnected_clients() {
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    size_t before = clients_connected.size();

    for (auto it = clients_connected.begin(); it != clients_connected.end();) {
        if (!(*it)->is_running()) {
            int client_id = (*it)->get_id();
            (*it)->stop_connection();
            delete *it;
            it = clients_connected.erase(it);

            std::cout << "[Acceptor] Client " << client_id 
                      << " disconnected and cleaned up" << std::endl;
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
    std::lock_guard<std::mutex> lock(clients_mutex);
    
    std::cout << "[Acceptor] Clearing all " << clients_connected.size() 
              << " connections..." << std::endl;

    for (auto* ch : clients_connected) {
        ch->stop_connection();
        delete ch;
    }
    clients_connected.clear();

    std::cout << "[Acceptor] All connections closed" << std::endl;
}

void Acceptor::stop_accepting() {
    std::cout << "[Acceptor] Stop signal received" << std::endl;
    is_running = false;
    is_accepting = false;
}

void Acceptor::run() {
    MatchesMonitor monitor;
    is_accepting = true;
    
    try {
        while (is_running && is_accepting) {
            manage_clients_connections(monitor);
            clear_disconnected_clients();
        }
    } catch (const std::exception& e) {
        if (is_running && is_accepting) {
            std::cerr << "[Acceptor] Error occurred: " << e.what() << std::endl;
        }
    }

    std::cout << "[Acceptor] Thread stopped" << std::endl;
}

void Acceptor::stop() {
    std::cout << "[Acceptor]  SHUTDOWN: Stopping acceptor..." << std::endl;
    
    is_running = false;
    is_accepting = false;
    
    // NO cerrar socket aquí - se hace desde Server::shutdown()
}

Acceptor::~Acceptor() {
    std::cout << "[Acceptor] Destructor called" << std::endl;
    
    // No llamar a stop() si ya se llamó
    if (is_running) {
        stop();
    }
    
    // Esperar a que termine el thread
    if (this->is_alive()) {
        this->join();
    }
    
    // Limpiar clientes
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (auto* client : clients_connected) {
        if (client) {
            delete client;
        }
    }
    clients_connected.clear();
    
    std::cout << "[Acceptor] Destructor completed" << std::endl;
}