#include "client_monitor.h"

#include <iostream>
#include <utility>

ClientMonitor::ClientMonitor() {}

void ClientMonitor::add_client_queue(Queue<GameState>& queue, int player_id) {
    std::lock_guard<std::mutex> lock(mtx);
    queues_list.push_back(std::make_pair(std::ref(queue), player_id));
}

void ClientMonitor::broadcast(const GameState& state) {
    std::lock_guard<std::mutex> lock(mtx);
    if (queues_list.empty()) {
        return;
    }

    for (auto& pair : queues_list) {
        Queue<GameState>& queue = pair.first;
        try {
            queue.try_push(state);
        } catch (const ClosedQueue&) {
        } catch (const std::exception&) {
        }
    }
}

void ClientMonitor::delete_client_queue(int player_id) {
    std::lock_guard<std::mutex> lock(mtx);
    // Recorremos la lista
    for (auto i = queues_list.begin(); i != queues_list.end();) {
        if (i->second == player_id) {
            // Borra el elemento.
            i = queues_list.erase(i);
            return;
        } else {
            // Si no eliminamos, avanzamos al siguiente elemento.
            ++i;
        }
    }
}
