#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <atomic>
#include <string>
#include <thread>
#include <vector>
#include <utility>
#include <map>
#include <QObject>
#include <QString> // Importante para las señales

#include "../../client_protocol.h"
#include "../../../common_src/dtos.h"

class LobbyClient : public QObject {
    Q_OBJECT

private:
    ClientProtocol& protocol;
    std::string username;
    std::atomic<bool> connected;
    std::atomic<bool> listening;
    std::thread notification_thread;

    void notification_listener();

public:
    explicit LobbyClient(ClientProtocol& protocol);
    ~LobbyClient();

    void send_username(const std::string& user);
    std::string receive_welcome();
    
    void request_games_list();
    std::vector<GameInfo> receive_games_list();
    
    void create_game(const std::string& game_name, uint8_t max_players, 
                     const std::vector<std::pair<std::string, std::string>>& races);
    uint16_t receive_game_created();
    
    void join_game(uint16_t game_id);
    uint16_t receive_game_joined();
    
    void receive_room_snapshot();
    void read_room_snapshot(std::vector<QString>& players, std::map<QString, QString>& cars);
    
    void select_car(const std::string& car_name, const std::string& car_type);
    std::string receive_car_confirmation();
    
    void start_game(uint16_t game_id);
    void leave_game(uint16_t game_id);
    void set_ready(bool is_ready);
    
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> receive_city_maps();
    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);
    
    void start_listening();
    // [MODIFICADO] flag shutdown_connection opcional
    void stop_listening(bool shutdown_connection = false);
    bool is_listening() const { return listening.load(); }

    // Métodos auxiliares
    uint8_t peek_message_type();
    void read_error_details(std::string& error_message);

signals:
    void playerJoinedNotification(QString username);
    void playerLeftNotification(QString username);
    void playerReadyNotification(QString username, bool isReady);
    void carSelectedNotification(QString username, QString carName, QString carType);
    void gameStartedNotification();
    void errorOccurred(QString message);
    void gamesListReceived(std::vector<GameInfo> games);
};

#endif // LOBBY_CLIENT_H
