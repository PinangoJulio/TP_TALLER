// server_src/server.cpp
#include "server.h"
#include <iostream>

Server::Server(const char* servicename) 
    : acceptor(servicename), shutdown_signal(false) {
    std::cout << "==================================================" << std::endl;
    std::cout << "    NEED FOR SPEED 2D - SERVER" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Port: " << servicename << std::endl;
    std::cout << "Press 'q' + Enter to stop" << std::endl;
    std::cout << "==================================================" << std::endl;
}

void Server::accept_connection() {
    std::cout << "[Server] Starting acceptor thread..." << std::endl;
    acceptor.start();
    std::cout << "[Server] Waiting for connections..." << std::endl;
}

void Server::shutdown() {
    std::cout << "\n==================================================" << std::endl;
    std::cout << "    🛑 SERVER SHUTDOWN INITIATED" << std::endl;
    std::cout << "==================================================" << std::endl;
    
    shutdown_signal = true;
    
    // 1. Detener el acceptor (no acepta más conexiones)
    std::cout << "[Server] Stopping acceptor..." << std::endl;
    acceptor.stop();
    
    std::cout << "[Server] ✅ Server shutdown complete" << std::endl;
    std::cout << "==================================================" << std::endl;
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