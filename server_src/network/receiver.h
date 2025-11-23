#ifndef SERVER_RECEIVER_H
#define SERVER_RECEIVER_H

#include <string>
#include <utility>
#include <vector>

#include "../../common_src/dtos.h"
#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "../../common_src/thread.h"
#include "common_src/game_state.h"
#include "matches_monitor.h"
#include "sender.h"
#include "server_src/server_protocol.h"

class Receiver : public Thread {
    ServerProtocol& protocol;
    int id;
    int match_id;
    std::string username;
    Queue<GameState>& sender_messages_queue;
    std::atomic<bool>& is_running;
    MatchesMonitor& monitor;
    Queue<ComandMatchDTO>* commands_queue = nullptr;
    Sender sender;

    bool handle_client_lobby();

public:
    Receiver(const Receiver& other) = delete;
    Receiver& operator=(const Receiver& other) = delete;

    Receiver(Receiver&& other) = default;

    explicit Receiver(ServerProtocol& protocol, int id, Queue<GameState>& sender_messages_queue,
                      std::atomic<bool>& is_running, MatchesMonitor& monitor);

    void run() override;
    void kill();
    bool status();

    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
    get_city_maps();
    void handle_lobby();
    void handle_match_messages();

    virtual ~Receiver();
};

#endif  // SERVER_RECEIVER_H
