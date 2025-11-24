#include "client_receiver.h"

ClientReceiver::ClientReceiver(ClientProtocol& protocol, Queue<GameState>& queue)
    : protocol(protocol), snapshots_queue(queue), client_id(-1) {}

void ClientReceiver::run() {
    while (should_keep_running()) {
        GameState game_state_snapshot;
        try {
            if (should_keep_running())
                game_state_snapshot = protocol.receive_snapshot();
            if (game_state_snapshot.race_info.status == MatchStatus::FINISHED)
                this->stop();
            InfoPlayer* player = game_state_snapshot.findPlayer(client_id);
            if (player && player->disconnected)
                this->stop();
        } catch (const std::exception& e) {
            break;
        }
        try {
            snapshots_queue.try_push(game_state_snapshot);
        } catch (const ClosedQueue&) {
            break;
        }
    }
}

ClientReceiver::~ClientReceiver() {
    stop();
    join();
}
