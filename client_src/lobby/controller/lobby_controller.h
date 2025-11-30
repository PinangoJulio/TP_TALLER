#ifndef LOBBY_CONTROLLER_H
#define LOBBY_CONTROLLER_H

#include <QMessageBox>
#include <QObject>
#include <QString>
#include <QTimer>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "../../client_protocol.h"
#include "../model/lobby_client.h"
#include "../view/common_types.h"
#include "../view/create_match_window.h"
#include "../view/garage_window.h"
#include "../view/lobby_window.h"
#include "../view/match_selection_window.h"
#include "../view/name_input_window.h"
#include "../view/waiting_room_window.h"

class LobbyClient;
class LobbyWindow;
class NameInputWindow;
class MatchSelectionWindow;
class CreateMatchWindow;
class GarageWindow;
class WaitingRoomWindow;

class LobbyController : public QObject {
    Q_OBJECT

private:
    // const char* serverHost;
    // const char* serverPort;

    ClientProtocol& protocol;
    std::unique_ptr<LobbyClient> lobbyClient;

    // Vistas
    LobbyWindow* lobbyWindow;
    NameInputWindow* nameInputWindow;
    MatchSelectionWindow* matchSelectionWindow;
    CreateMatchWindow* createMatchWindow;
    GarageWindow* garageWindow;
    WaitingRoomWindow* waitingRoomWindow;

    // Estado
    QString playerName;
    uint16_t currentGameId;
    int selectedCarIndex;
    std::vector<QString> pendingPlayers;
    std::map<QString, QString> pendingCars;

    // Estado del ciclo de lobby
    bool lobbyCompleted = false;
    bool lobbySuccess = false;

public:
    explicit LobbyController(ClientProtocol& protocol, QObject* parent = nullptr);
    ~LobbyController();

    // Iniciar el flujo (mostrar lobby principal)
    void start();

    // Cerrar todas las ventanas de Qt (llamar DESPUÉS de que QEventLoop termine)
    void closeAllWindows();

    bool isLobbySuccessful() const { return lobbyCompleted && lobbySuccess; }
    QString getPlayerName() const { return playerName; }
signals:
    void lobbyFinished(bool success);
    void raceInfoReceived(RaceInfoDTO info); 


private slots:
    // usuario presiona "Jugar" en el lobby
    void onPlayClicked();

    // usuario confirma su nombre
    void onNameConfirmed(const QString& name);

    // usuario presiona "Volver" desde el nombre
    void onBackFromNameInput();

    // Match Selection
    void onRefreshMatchList();
    void onJoinMatchRequested(const QString& matchId);
    void onCreateMatchRequested();
    void onBackFromMatchSelection();

    // Create Match
    void onMatchCreated(const QString& matchName, int maxPlayers,
                        const std::vector<RaceConfig>& races);
    void onBackFromCreateMatch();

    // Garage
    void onCarSelected(const CarInfo& car);
    void onBackFromGarage();

    // Waiting Room
    void onPlayerReadyToggled(bool isReady);
    void onStartGameRequested();
    void onBackFromWaitingRoom();

private:
    void connectToServer();
    void handleNetworkError(const std::exception& e);
    void cleanupAndReturnToLobby();
    void openMatchSelection();
    void refreshGamesList();
    void openGarage();
    void openWaitingRoom();
    void connectNotificationSignals();
    bool waiting_for_race_info = false;

    void finishLobby(bool success) {
        if (!lobbyCompleted) {
            lobbyCompleted = true;
            lobbySuccess = success;
            emit lobbyFinished(success);
        }
    }
};

#endif  // LOBBY_CONTROLLER_H
