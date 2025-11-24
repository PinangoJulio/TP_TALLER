
#ifndef CLIENT_RECEIVER_H
#define CLIENT_RECEIVER_H
#include "client_protocol.h"
#include "common_src/game_state.h"
#include "common_src/queue.h"
#include "common_src/thread.h"

class ClientReceiver : public Thread {
private:
    ClientProtocol& protocol;
    Queue<GameState>& snapshots_queue;
    int client_id;

public:
    explicit ClientReceiver(ClientProtocol& protocolo, Queue<GameState>& queue);
    void run() override;
    bool esta_vivo() const { return should_keep_running(); }
    void set_id(int id) { client_id = id; }

    ClientReceiver(const ClientReceiver&) = delete;
    ClientReceiver& operator=(const ClientReceiver&) = delete;
    ClientReceiver(ClientReceiver&&) = default;
    ClientReceiver& operator=(ClientReceiver&&) = default;

    ~ClientReceiver();
};
#endif  // CLIENT_RECEIVER_H
