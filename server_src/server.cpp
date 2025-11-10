#include "server.h"

#include <iostream>

#include "../common_src/queue.h"
#include "../common_src/dtos.h"

#include "network/monitor.h"
#include "game/game_loop.h"

#define QUIT 'q'

Server::Server(const char *servicename): acceptor(servicename){}

void Server::accept_connection() {
    acceptor.start();
}

void Server::start() {

    accept_connection();
    while (std::cin.get() != QUIT) {}

}

Server::~Server() {
    acceptor.stop_accepting();
}
