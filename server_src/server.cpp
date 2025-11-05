#include "server.h"

#include <iostream>

#include "../common_src/queue.h"
#include "../common_src/dtos.h"

#include "acceptor.h"
#include "network/monitor.h"
#include "game/game_loop.h"  // ✅ AGREGAR ESTA LÍNEA

Server::Server(const std::string& servicename): servicename(servicename) {}

void Server::run() {
    Monitor monitor;
    Queue<struct Command> game_queue;
    Acceptor acceptor(this->servicename, monitor, game_queue);

    acceptor.start();
    GameSimulator game(monitor, game_queue);  // ✅ Ahora sí conoce GameSimulator
    game.start();
    
    while (std::cin.get() != QUIT) {}

    game.stop();
    game.join();
    acceptor.stop();
    acceptor.join();
}