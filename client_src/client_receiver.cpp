#include "client_receiver.h"

ClientReceiver::ClientReceiver(ClientProtocol& protocol, Queue<GameState>& queue)
    : protocol(protocol), snapshots_queue(queue), client_id(-1) {}

    void ClientReceiver::run() {

        while (should_keep_running()) {
            GameState game_state_snapshot;
            try {
                if (!should_keep_running()) break;
    
                game_state_snapshot = protocol.receive_snapshot();
    
                if (game_state_snapshot.players.empty()) {
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
                
                if (error_msg.find("Server shutdown") != std::string::npos) {

                    snapshots_queue.close();
                    this->stop();

                    throw;
                }
                
                if (should_keep_running()) {
                    std::cerr << "[ClientReceiver]  Error recibiendo snapshot: " << e.what() << std::endl;
                }
                break;
            }
            
            try {
                snapshots_queue.try_push(game_state_snapshot);
            } catch (const ClosedQueue&) {
                break;
            } catch (const std::exception& e) {
                if (should_keep_running()) {
                     std::cerr << "[ClientReceiver] Warning al pushear snapshot: " << e.what() << std::endl;
                }
                break;
            }
        }
    
    }

ClientReceiver::~ClientReceiver() {
    // El join() se hace desde Client::~Client()
    // No hacemos nada aquí para evitar doble join
}
