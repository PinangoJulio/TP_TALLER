#include "server.h"

#include <iostream>

#include "../common_src/queue.h"
#include "../common_src/dtos.h"

#include "network/monitor.h"
#include "game/game_loop.h"

#define QUIT 'q'

Server::Server(const char *servicename): acceptor(servicename){}

<<<<<<< HEAD
void Server::run() {
    Configuracion config("config/configuracion.yaml");
    Monitor monitor;
    Queue<struct Command> game_queue;
    Acceptor acceptor(this->servicename, monitor, game_queue);
}

void Server::accept_connection() {
    acceptor.start();
    GameSimulator game(monitor, game_queue, config); 
    game.start();
    
}

void Server::start() {

    accept_connection();
    while (std::cin.get() != QUIT) {}

}

Server::~Server() {
    acceptor.stop_accepting();
}
