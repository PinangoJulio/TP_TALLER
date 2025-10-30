#include "server.h"

#include <iostream>

#include "../common_src/queue.h"
#include "../common_src/utils.h"

#include "server_acceptor.h"
#include "server_game.h"
#include "server_monitor.h"


Server::Server(const std::string& servicename): servicename(servicename) {}

void Server::run() {
    Monitor monitor;
    Queue<struct Command> game_queue;
    Acceptor acceptor(this->servicename, monitor, game_queue);

    acceptor.start();
    GameSimulator game(monitor, game_queue);
    game.start();
    while (std::cin.get() != QUIT) {}

    game.stop();
    game.join();
    acceptor.stop();
    acceptor.join();
}
