#ifndef CLIENT_H
#define CLIENT_H

#include <string>

#include "client_protocol.h"
#include "client_receiver.h"
#include "client_sender.h"
#include "common_src/dtos.h"
#include "common_src/game_state.h"
#include "common_src/queue.h"

class Client {
private:
    ClientProtocol protocol;
    std::string username;
    uint16_t player_id;
    std::vector<std::string> races_paths;
    bool active;

    // Queues para comunicación entre threads
    Queue<ComandMatchDTO> command_queue;  // Comandos del jugador → servidor
    Queue<GameState> snapshot_queue;      // Snapshots del servidor → renderizado

    // Threads de comunicación
    ClientSender sender;
    ClientReceiver receiver;

    bool threads_started;

public:
    explicit Client(const char* hostname, const char* servname);

    // Iniciar el cliente (lobby + juego)
    void start();

    // No copiable, movible por defecto
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&) = default;
    Client& operator=(Client&&) = default;

    ~Client();
};

#endif  // CLIENT_H
