#include "client_handler.h"

#include <utility>
#include <sys/socket.h>


ClientHandler::ClientHandler(Socket skt, int id, MatchesMonitor& monitor)
    : skt(std::move(skt)), client_id(id), protocol(this->skt), monitor(monitor), is_alive(true),
      messages_queue(), receiver(protocol, this->client_id, messages_queue, is_alive, monitor) {}

      void ClientHandler::send_shutdown_message(const std::vector<uint8_t>& msg) {
        try {

            // Enviar mensaje directamente por el socket
            skt.sendall(msg.data(), msg.size());
            
        } catch (const std::exception& e) {
            std::cerr << "[ClientHandler " << client_id 
                      << "] Error sending shutdown: " << e.what() << std::endl;
        }
    }

void ClientHandler::run_threads() {
    receiver.start();
}

void ClientHandler::stop_connection() {
    is_alive = false;
    
    // Cerrar colas
    try {
        messages_queue.close();
    } catch (...) {}

    // Matar receiver primero
    receiver.kill();
    
    try {
        skt.shutdown(SHUT_RDWR);
    } catch (...) {
        // Ignorar errores si ya estaba cerrado
    }

    std::cout << "[ClientHandler " << client_id << "]  Shutdown initiated" << std::endl;
}

bool ClientHandler::is_running() {
    return is_alive;
}

ClientHandler::~ClientHandler() {
    stop_connection();

    try {
        receiver.join();
    } catch (const std::exception& e) {
        std::cerr << "[ClientHandler " << client_id << "] Error joining receiver: "
                  << e.what() << std::endl;
    }

    try {
        skt.close();
    } catch (...) {
        // Ignorar si ya estaba cerrado
    }

}