#ifndef LOBBY_CONTROLLER_H
#define LOBBY_CONTROLLER_H

#include <memory>
#include <vector>
#include <QObject>
#include <QString>
#include <iostream>
#include <QMessageBox>

#include "../view/common_types.h" 
#include "../view/lobby_window.h"
#include "../model/lobby_client.h"
#include "../view/garage_window.h"
#include "../view/name_input_window.h"
#include "../view/waiting_room_window.h"
#include "../view/create_match_window.h"
#include "../view/match_selection_window.h"


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
   
    QString serverHost;
    QString serverPort;
    
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

public:
    explicit LobbyController(const QString& host, const QString& port, QObject* parent = nullptr);
    ~LobbyController();
    
    // Iniciar el flujo (mostrar lobby principal)
    void start();

private slots:
    //usuario presiona "Jugar" en el lobby
    void onPlayClicked();
    
    //usuario confirma su nombre
    void onNameConfirmed(const QString& name);
    
    //usuario presiona "Volver" desde el nombre
    void onBackFromNameInput();
    
    //Match Selection
    void onRefreshMatchList();
    void onJoinMatchRequested(const QString& matchId);
    void onCreateMatchRequested();
    void onBackFromMatchSelection();
    
    //Create Match
    void onMatchCreated(const QString& matchName, int maxPlayers, const std::vector<RaceConfig>& races);
    void onBackFromCreateMatch();
    
    //Garage
    void onCarSelected(const CarInfo& car);
    void onBackFromGarage();
    
    //Waiting Room
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
};

#endif // LOBBY_CONTROLLER_H