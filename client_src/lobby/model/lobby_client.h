#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <QObject>
#include <atomic>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "client_src/client_protocol.h"
#include "client_src/lobby/view/common_types.h"
#include "common_src/lobby_protocol.h"
#include "common_src/socket.h"

class LobbyClient : public QObject {
    Q_OBJECT

private:
    // Socket socket;
    ClientProtocol protocol;
    std::string username;
    bool connected;

    std::atomic<bool> listening;
    std::thread notification_thread;

    // Helpers de lectura

    void receive_room_snapshot();

    void notification_listener();

public:
    LobbyClient(const char* host, const char* port);
    ~LobbyClient();

    // === LOBBY ===
    void send_username(const std::string& username);
    std::string receive_welcome();
    void request_games_list();
    std::vector<GameInfo> receive_games_list();

    // === MATCH ===
    void create_game(const std::string& game_name, uint8_t max_players,
                     const std::vector<std::pair<std::string, std::string>>& races);
    uint16_t receive_game_created();
    void join_game(uint16_t game_id);
    uint16_t receive_game_joined();
    void leave_game(uint16_t game_id);

    // === GARAGE ===
    void select_car(const std::string& car_name, const std::string& car_type);
    std::string receive_car_confirmation();

    // : Waiting Room
    void set_ready(bool is_ready);
    void start_game(uint16_t game_id);

    //  Control del thread receptor
    void start_listening();
    void stop_listening();

    // Error handling
    uint8_t peek_message_type();
    void read_error_details(std::string& error_message);

    // Getters
    bool is_connected() const { return connected; }
    std::string get_username() const { return username; }

    // No copiable
    LobbyClient(const LobbyClient&) = delete;
    LobbyClient& operator=(const LobbyClient&) = delete;

    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
    receive_city_maps();

    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);

    bool is_listening() const { return listening.load(); }

    void read_room_snapshot(std::vector<QString>& players, std::map<QString, QString>& cars);

signals:
    //  Señales para notificar cambios en la sala
    void playerJoinedNotification(QString username);
    void playerLeftNotification(QString username);
    void playerReadyNotification(QString username, bool isReady);
    void carSelectedNotification(QString username, QString carName, QString carType);
    void gameStartedNotification();
    void errorOccurred(QString errorMessage);
    void gamesListReceived(std::vector<GameInfo> games);
};

#endif  // LOBBY_CLIENT_H
