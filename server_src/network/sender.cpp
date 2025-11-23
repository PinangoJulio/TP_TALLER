#include "sender.h"

#include <iostream>

Sender::Sender(ServerProtocol& protocol, Queue<GameState>& sender_queue, std::atomic<bool>& alive,
               int player_id)
    : protocol(protocol), sender_queue(sender_queue), alive(alive), player_id(player_id) {}

void Sender::run() {
    try {
        while (alive) {
            GameState snapshot = sender_queue.pop();
            // protocol.send_game_state(snapshot);  --> enviar logica
        }
    } catch (...) {
        alive = false;
    }
}

Sender::~Sender() {}
