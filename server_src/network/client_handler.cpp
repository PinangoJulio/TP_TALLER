#include "client_handler.h"

#include <utility>

ClientHandler::ClientHandler(Socket skt, int id, MatchesMonitor& monitor)
    : skt(std::move(skt)), client_id(id), protocol(this->skt), monitor(monitor), is_alive(true),
      messages_queue(), receiver(protocol, this->client_id, messages_queue, is_alive, monitor) {}

void ClientHandler::run_threads() {
    receiver.start();
}

void ClientHandler::stop_connection() {
    std::cout << "[ClientHandler " << client_id << "] 🛑 Shutdown signal received" << std::endl;
    
    is_alive = false;
    
    // Cerrar las colas para desbloquear los threads
    try {
        messages_queue.close();
    } catch (...) {
        // Ya estaba cerrada
    }
    
    // Detener el receiver
    receiver.kill();
    
    std::cout << "[ClientHandler " << client_id << "] Shutdown initiated" << std::endl;
}

bool ClientHandler::is_running() {
    return is_alive;
}

ClientHandler::~ClientHandler() {
    std::cout << "[ClientHandler " << client_id << "] Destructor called" << std::endl;
    
    stop_connection();
    
    // Esperar a que termine el receiver
    if (receiver.is_alive()) {
        std::cout << "[ClientHandler " << client_id << "] Waiting for receiver..." << std::endl;
        receiver.join();
    }
    
    std::cout << "[ClientHandler " << client_id << "] ✅ Destroyed" << std::endl;
}
