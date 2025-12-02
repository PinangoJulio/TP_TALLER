#include "client_receiver.h"

ClientReceiver::ClientReceiver(ClientProtocol& protocol, Queue<GameState>& queue)
    : protocol(protocol), snapshots_queue(queue), client_id(-1) {}

    void ClientReceiver::run() {
        std::cout << "[ClientReceiver] 📡 Thread iniciado, esperando snapshots..." << std::endl;
    
        while (should_keep_running()) {
            GameState game_state_snapshot;
            try {
                if (!should_keep_running()) break;
    
                game_state_snapshot = protocol.receive_snapshot();
    
                if (game_state_snapshot.players.empty()) {
                    std::cout << "[ClientReceiver] Snapshot vacío recibido, esperando..." << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }
    
                if (game_state_snapshot.race_info.status == MatchStatus::FINISHED)
                    this->stop();
                
                InfoPlayer* player = game_state_snapshot.findPlayer(client_id);
                if (player && player->disconnected)
                    this->stop();
                    
            } catch (const std::runtime_error& e) {
                std::string error_msg = e.what();
                
                // ✅ Detectar shutdown y propagar
                if (error_msg.find("Server shutdown") != std::string::npos) {
                    std::cout << "[ClientReceiver] 🛑 Server shutdown detected - stopping client" << std::endl;
                    
                    // ✅ NO hacer try_push - cerrar la cola y salir
                    snapshots_queue.close();
                    this->stop();
                    
                    // ✅ Re-lanzar la excepción para que Client::start() la capture
                    throw;
                }
                
                if (should_keep_running()) {
                    std::cerr << "[ClientReceiver] ❌ Error recibiendo snapshot: " << e.what() << std::endl;
                }
                break;
            }
            
            try {
                snapshots_queue.try_push(game_state_snapshot);
            } catch (const ClosedQueue&) {
                break;
            }
        }
    
        std::cout << "[ClientReceiver] 📡 Thread finalizado" << std::endl;
    }

ClientReceiver::~ClientReceiver() {
    // El join() se hace desde Client::~Client()
    // No hacemos nada aquí para evitar doble join
}
