#ifndef SERVER_RECEIVER_H
#define SERVER_RECEIVER_H

#include "matches_monitor.h"
#include "sender.h"
#include "../../common_src/queue.h"
#include "../../common_src/socket.h"
#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"
#include "common_src/game_state.h"
#include "server_src/server_protocol.h"

/* Thread Receiver
 *  Espera mensajes por socket, los deserializa y manda hacia la queue del juego
 */
class Receiver: public Thread {
    ServerProtocol& protocol;
    int id;
    int match_id;
    Queue<GameState>& sender_messages_queue;
    std::atomic<bool>& is_running;
    MatchesMonitor& monitor;
    Queue<struct Command>* commands_queue = nullptr;  // recibidora del game_queue
    Sender sender;
    LobbyManager& lobby_manager;

    bool handle_client_lobby();
public:
    explicit Receiver(ServerProtocol& protocol, int id, Queue<GameState>& sender_messages_queue, std::atomic<bool>& is_running, MatchesMonitor& monitor, LobbyManager& lobby_manager);

    void run() override;
    void kill();
    bool status();

    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> get_city_maps();
    void handle_lobby();
    void handle_match_messages();

    virtual ~Receiver();
};

#endif  // SERVER_RECEIVER_H
