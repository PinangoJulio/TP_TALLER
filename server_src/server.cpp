#include "server.h"
#include <iostream>

#define QUIT 'q'

Server::Server(const char* servicename) 
    : acceptor(servicename) {}

void Server::accept_connection() {
    acceptor.start();
}

void Server::start() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "    NEED FOR SPEED 2D - SERVER" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Presiona 'q' + Enter para salir." << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    accept_connection();
    
    while (std::cin.get() != QUIT) {}
    
    std::cout << "\nApagando servidor..." << std::endl;
}

Server::~Server() {
    acceptor.stop_accepting();
}
