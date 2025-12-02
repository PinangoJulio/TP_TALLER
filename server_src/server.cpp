#include "server.h"
#include <iostream>

Server::Server(const char* servicename) 
    : acceptor(servicename), shutdown_signal(false) {

}

void Server::accept_connection() {
    std::cout << "[Server] Starting acceptor thread..." << std::endl;
    acceptor.start();
    std::cout << "[Server] Waiting for connections..." << std::endl;
}

void Server::shutdown() {
    if (shutdown_signal) {
        return;
    }
    

    shutdown_signal = true;
    
    
    // 1. Señalizar cierre (para que dejen de aceptar nuevas conexiones)
    acceptor.stop_accepting();
    
    // 2. Notificar a TODOS los clientes conectados
    acceptor.notify_shutdown_to_all_clients();
    
    // 3. Dar tiempo para que los mensajes lleguen
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // 4. Cerrar el socket del acceptor (desbloquea accept())
    std::cout << "[Server] Closing acceptor socket..." << std::endl;
    acceptor.close_socket();
    
    // 5. Esperar a que termine el thread
    std::cout << "[Server] Waiting for acceptor thread..." << std::endl;
    if (acceptor.is_alive()) {
        acceptor.join();
    }
    
    std::cout << "[Server]   Server shutdown complete" << std::endl;
}

void Server::start() {
    accept_connection();

    char input;
    while (std::cin.get(input)) {
        if (input == 'q' || input == 'Q') {
            break;
        }
    }

    shutdown();
}

Server::~Server() {
    if (!shutdown_signal) {
        shutdown();
    }
}