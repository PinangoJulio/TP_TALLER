#ifndef LOBBY_CLIENT_H
#define LOBBY_CLIENT_H

#include <QObject>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <map> // Necesario para std::map
#include <utility> // Necesario para std::pair

#include "../../client_protocol.h" 
#include "../../../common_src/dtos.h" 

class LobbyClient : public QObject {
    Q_OBJECT

private:
    ClientProtocol& protocol;
    std::atomic<bool> listening{false};
    std::thread notification_thread;
    bool connected;
    std::string username;

    void notification_listener();

public:
    explicit LobbyClient(ClientProtocol& protocol);
    ~LobbyClient();

    // -- Métodos de conexión y configuración --
    void send_username(const std::string& user);
    std::string receive_welcome();
    
    // -- Gestión de lista de partidas --
    void request_games_list();
    std::vector<GameInfo> receive_games_list();
    
    // -- Creación de partidas --
    // Nota: Declaramos el vector complejo para que coincida con tu .cpp
    void create_game(const std::string& game_name, uint8_t max_players,
                     const std::vector<std::pair<std::string, std::string>>& races);
    uint16_t receive_game_created();
    
    // -- Gestión de unirse a partidas --
    void join_game(uint16_t game_id);
    uint16_t receive_game_joined();
    
    // -- Snapshots y estado de sala --
    // Versión manual (síncrona) que usa el controller al entrar
    void read_room_snapshot(std::vector<QString>& players, std::map<QString, QString>& cars);
    
    // Versión que existía en tu .cpp (probablemente antigua o para uso interno)
    void receive_room_snapshot(); 

    // -- Selección de auto y estado --
    void select_car(const std::string& car_name, const std::string& car_type);
    std::string receive_car_confirmation();
    void set_ready(bool is_ready);
    
    // -- Inicio y fin de juego --
    void start_game(uint16_t game_id);
    void leave_game(uint16_t game_id);
    
    // -- Control del Listener --
    void start_listening();
    void stop_listening(bool shutdown_connection);
    bool is_listening() const { return listening.load(); }

    // -- Métodos auxiliares faltantes que causaban error --
    uint8_t peek_message_type();
    void read_error_details(std::string& error_message);
    
    // Funciones para mapas y configuración de carreras
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> receive_city_maps();
    void send_selected_races(const std::vector<std::pair<std::string, std::string>>& races);

signals:
    // Señales para la UI de Qt
    void playerJoinedNotification(QString username);
    void playerLeftNotification(QString username);
    void playerReadyNotification(QString username, bool isReady);
    void carSelectedNotification(QString username, QString carName, QString carType);
    void gamesListReceived(std::vector<GameInfo> games);
    void errorOccurred(QString errorMsg);

    // NUEVA SEÑAL (Esencial para arreglar tu bug de inicio)
    void gameStartedNotification();
};

#endif // LOBBY_CLIENT_H
