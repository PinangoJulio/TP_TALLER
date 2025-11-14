#include "server.h"

#include <iostream>

#include "../common_src/queue.h"
#include "../common_src/dtos.h"

#include "network/monitor.h"
#include "game/game_loop.h"

#define QUIT 'q'

Server::Server(const char *servicename): acceptor(servicename){
    // 🔥 AGREGADO: Banner de inicio
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

void Server::start() {
    accept_connection();
    
    while (std::cin.get() != QUIT) {
        std::cout << "[Server] Press 'q' to quit..." << std::endl;
    }
    
    std::cout << "[Server] Shutting down..." << std::endl;
}

Server::~Server() {
    std::cout << "[Server] Stopping acceptor..." << std::endl;
    acceptor.stop_accepting();
    std::cout << "[Server] Server stopped." << std::endl;
}