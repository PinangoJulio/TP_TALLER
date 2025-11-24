#include "monitor.h"

Monitor::Monitor() {}

void Monitor::add_client_to_queue(Queue<struct ServerMsg>& client_queue) {
    std::unique_lock<std::mutex> lck(mtx);
    message_queue.push_back(&client_queue);
}

void Monitor::broadcast(const ServerMsg& server_msg) {
    std::unique_lock<std::mutex> lck(mtx);
    for (auto* q: message_queue) {
        try {
            q->push(server_msg);
        } catch (const std::exception& e) {
            // Queue cerrada, ignorar
        }
    }
}

void Monitor::clean_queue() {
    std::unique_lock<std::mutex> lck(mtx);
    message_queue.clear();
}
