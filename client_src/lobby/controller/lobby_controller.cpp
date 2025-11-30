#include "lobby_controller.h"

#include <map>
#include <string>
#include <utility>

#include <QApplication>
#include <QMessageBox>

LobbyController::LobbyController(ClientProtocol& protocol, QObject* parent)
    : QObject(parent), protocol(protocol), lobbyWindow(nullptr), nameInputWindow(nullptr),
      matchSelectionWindow(nullptr), createMatchWindow(nullptr), garageWindow(nullptr),
      waitingRoomWindow(nullptr), currentGameId(0), selectedCarIndex(-1) {
    std::cout << "[Controller] Controlador creado" << std::endl;
}

LobbyController::~LobbyController() {
    std::cout << "[Controller] Destructor llamado" << std::endl;
}

void LobbyController::closeAllWindows() {
    std::cout << "[Controller] Cerrando ventanas de Qt..." << std::endl;

    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        matchSelectionWindow->deleteLater();
        matchSelectionWindow = nullptr;
    }
    if (createMatchWindow) {
        createMatchWindow->close();
        createMatchWindow->deleteLater();
        createMatchWindow = nullptr;
    }
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    if (lobbyWindow) {
        lobbyWindow->close();
        lobbyWindow->deleteLater();
        lobbyWindow = nullptr;
    }
}

void LobbyController::start() {
    std::cout << "[Controller] Mostrando lobby principal" << std::endl;
    lobbyWindow = new LobbyWindow();
    connect(lobbyWindow, &LobbyWindow::playRequested, this, &LobbyController::onPlayClicked);
    lobbyWindow->show();
}

void LobbyController::onPlayClicked() {
    lobbyWindow->hide();
    try {
        connectToServer();
        nameInputWindow = new NameInputWindow();
        connect(nameInputWindow, &NameInputWindow::nameConfirmed, this,
                &LobbyController::onNameConfirmed);
        nameInputWindow->show();
    } catch (const std::exception& e) {
        handleNetworkError(e);
        lobbyWindow->show();
    }
}

void LobbyController::connectToServer() {
    lobbyClient = std::make_unique<LobbyClient>(protocol);
    std::cout << "[Controller] Conectado exitosamente" << std::endl;
}

void LobbyController::onNameConfirmed(const QString& name) {
    playerName = name;
    try {
        lobbyClient->send_username(name.toStdString());
        std::string welcome = lobbyClient->receive_welcome();
        std::cout << "[Controller] Bienvenida: " << welcome << std::endl;

        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;

        openMatchSelection();
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromNameInput() {
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    lobbyClient.reset();
    lobbyWindow->show();
}

void LobbyController::handleNetworkError(const std::exception& e) {
    std::cerr << "[Controller] ❌ Error: " << e.what() << std::endl;

    QWidget* currentWindow = lobbyWindow;
    if (waitingRoomWindow && waitingRoomWindow->isVisible()) currentWindow = waitingRoomWindow;
    else if (garageWindow && garageWindow->isVisible()) currentWindow = garageWindow;
    else if (createMatchWindow && createMatchWindow->isVisible()) currentWindow = createMatchWindow;
    else if (matchSelectionWindow && matchSelectionWindow->isVisible()) currentWindow = matchSelectionWindow;
    else if (nameInputWindow && nameInputWindow->isVisible()) currentWindow = nameInputWindow;

    QString errorMsg = QString::fromStdString(e.what());

    if (errorMsg.contains("Connection closed") || errorMsg.contains("closed by server") ||
        errorMsg.contains("EOF")) {
        QMessageBox::critical(currentWindow, "Desconectado", "El servidor cerró la conexión.");
        cleanupAndReturnToLobby();
    } else {
        QMessageBox::critical(currentWindow, "Error de Red", errorMsg);
    }
}

void LobbyController::cleanupAndReturnToLobby() {
    if (waitingRoomWindow) { waitingRoomWindow->close(); waitingRoomWindow->deleteLater(); waitingRoomWindow = nullptr; }
    if (garageWindow) { garageWindow->close(); garageWindow->deleteLater(); garageWindow = nullptr; }
    if (createMatchWindow) { createMatchWindow->close(); createMatchWindow->deleteLater(); createMatchWindow = nullptr; }
    if (matchSelectionWindow) { matchSelectionWindow->close(); matchSelectionWindow->deleteLater(); matchSelectionWindow = nullptr; }
    if (nameInputWindow) { nameInputWindow->close(); nameInputWindow->deleteLater(); nameInputWindow = nullptr; }

    lobbyClient.reset();
    currentGameId = 0;
    selectedCarIndex = -1;
    playerName.clear();

    if (lobbyWindow) lobbyWindow->show();
}

void LobbyController::onMatchCreated(const QString& matchName, int maxPlayers,
                                     const std::vector<RaceConfig>& races) {
    std::cout << "[Controller] Creando partida..." << std::endl;
    
    
    if (createMatchWindow) {
        createMatchWindow->showLoading("PREPARANDO DATOS...", 10);
    }

    std::vector<std::pair<std::string, std::string>> race_pairs;
    race_pairs.reserve(races.size());
    for (const auto& race : races) {
        race_pairs.emplace_back(race.cityName.toStdString(), race.trackName.toStdString());
    }

    try {
       
        if (createMatchWindow) {
            createMatchWindow->showLoading("CONTACTANDO SERVIDOR...", 30);
        }

        lobbyClient->create_game(matchName.toStdString(), static_cast<uint8_t>(maxPlayers),
                                 race_pairs);

       
        if (createMatchWindow) {
            createMatchWindow->showLoading("ESPERANDO CONFIRMACIÓN...", 60);
        }

        currentGameId = lobbyClient->receive_game_created();
        std::cout << "[Controller] Partida creada ID: " << currentGameId << std::endl;

       
        if (createMatchWindow) {
            createMatchWindow->showLoading("CARGANDO RECURSOS...", 80);
        }

        if (!lobbyClient->is_listening()) {
            connectNotificationSignals();
        }

       
        openGarage(); 

        // Una vez que termina openGarage, cerramos la ventana
        if (createMatchWindow) {
            createMatchWindow->close();
            createMatchWindow->deleteLater();
            createMatchWindow = nullptr;
        }

    } catch (const std::exception& e) {
        if (createMatchWindow) {
             createMatchWindow->close();
             createMatchWindow->deleteLater();
             createMatchWindow = nullptr;
        }
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromCreateMatch() {
    if (createMatchWindow) {
        createMatchWindow->close();
        createMatchWindow->deleteLater();
        createMatchWindow = nullptr;
    }
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
    }
}

void LobbyController::openMatchSelection() {
    matchSelectionWindow = new MatchSelectionWindow();
    connect(matchSelectionWindow, &MatchSelectionWindow::joinMatchRequested, this,
            &LobbyController::onJoinMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::createMatchRequested, this,
            &LobbyController::onCreateMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::backToLobby, this,
            &LobbyController::onBackFromMatchSelection);
    connect(matchSelectionWindow, &MatchSelectionWindow::refreshRequested, this,
            &LobbyController::onRefreshMatchList);

    matchSelectionWindow->show();
    refreshGamesList();
}

void LobbyController::refreshGamesList() {
    try {
        lobbyClient->request_games_list();
        std::vector<GameInfo> games = lobbyClient->receive_games_list();
        if (matchSelectionWindow) {
            matchSelectionWindow->updateGamesList(games);
        }
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onRefreshMatchList() {
    refreshGamesList();
}

void LobbyController::onJoinMatchRequested(const QString& matchId) {
    (void)matchId; // Evitamos warning de variable no usada

    if (!matchSelectionWindow) return;

    QListWidgetItem* selectedItem = matchSelectionWindow->getSelectedItem();
    if (!selectedItem) {
        QMessageBox::warning(matchSelectionWindow, "Selección requerida",
                             "Por favor selecciona una partida");
        return;
    }

    uint16_t gameId = selectedItem->data(Qt::UserRole).toUInt();

    try {
        lobbyClient->join_game(gameId);
        currentGameId = lobbyClient->receive_game_joined();
        
        std::vector<QString> snapshotPlayers;
        std::map<QString, QString> snapshotCars;
        lobbyClient->read_room_snapshot(snapshotPlayers, snapshotCars);

        pendingPlayers = snapshotPlayers;
        pendingCars = snapshotCars;

        matchSelectionWindow->hide();

        if (!lobbyClient->is_listening()) {
            connectNotificationSignals();
        }

        openGarage();

    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onCreateMatchRequested() {
    if (matchSelectionWindow) {
        matchSelectionWindow->hide();
    }

    createMatchWindow = new CreateMatchWindow();
    connect(createMatchWindow, &CreateMatchWindow::matchCreated, this,
            &LobbyController::onMatchCreated);
    connect(createMatchWindow, &CreateMatchWindow::backRequested, this,
            &LobbyController::onBackFromCreateMatch);

    createMatchWindow->show();
}

void LobbyController::onBackFromMatchSelection() {
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        matchSelectionWindow->deleteLater();
        matchSelectionWindow = nullptr;
    }
    lobbyClient.reset();
    if (lobbyWindow) lobbyWindow->show();
}

void LobbyController::openGarage() {
    garageWindow = new GarageWindow();
    connect(garageWindow, &GarageWindow::carSelected, this, &LobbyController::onCarSelected);
    connect(garageWindow, &GarageWindow::backRequested, this, &LobbyController::onBackFromGarage);
    garageWindow->show();
}

void LobbyController::onCarSelected(const CarInfo& car) {
    try {
        lobbyClient->select_car(car.name.toStdString(), car.type.toStdString());
        lobbyClient->receive_car_confirmation();

        if (garageWindow) {
            garageWindow->close();
            garageWindow->deleteLater();
            garageWindow = nullptr;
        }

        if (!lobbyClient->is_listening()) {
            lobbyClient->start_listening();
        }

        openWaitingRoom();

        if (waitingRoomWindow) {
            waitingRoomWindow->setPlayerCarByName(playerName, car.name);
        }
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::connectNotificationSignals() {
    if (lobbyClient) {
        disconnect(lobbyClient.get(), nullptr, this, nullptr);
    }

    connect(lobbyClient.get(), &LobbyClient::playerJoinedNotification, this,
            [this](QString username) {
                if (waitingRoomWindow) waitingRoomWindow->addPlayerByName(username);
                else pendingPlayers.push_back(username);
            });

    connect(lobbyClient.get(), &LobbyClient::playerLeftNotification, this, [this](QString username) {
        if (waitingRoomWindow) waitingRoomWindow->removePlayerByName(username);
    });

    connect(lobbyClient.get(), &LobbyClient::playerReadyNotification, this,
            [this](QString username, bool isReady) {
                if (waitingRoomWindow) waitingRoomWindow->setPlayerReadyByName(username, isReady);
            });

    connect(lobbyClient.get(), &LobbyClient::carSelectedNotification, this,
            [this](QString username, QString carName, QString) {
                if (waitingRoomWindow) waitingRoomWindow->setPlayerCarByName(username, carName);
                else pendingCars[username] = carName;
            });

    connect(lobbyClient.get(), &LobbyClient::gameStartedNotification, this, [this]() {
        if (waitingRoomWindow) waitingRoomWindow->close();
    });

    connect(lobbyClient.get(), &LobbyClient::errorOccurred, this, [this](QString errorMsg) {
        QMessageBox::critical(waitingRoomWindow, "Error", errorMsg);
    });

    connect(lobbyClient.get(), &LobbyClient::gamesListReceived, this,
            [this](std::vector<GameInfo> games) {
                if (matchSelectionWindow) matchSelectionWindow->updateGamesList(games);
            });
}

void LobbyController::onBackFromGarage() {
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }

    if (lobbyClient && currentGameId > 0) {
        try {
            lobbyClient->leave_game(currentGameId);
        } catch (const std::exception& e) {
            std::cerr << "Error leave: " << e.what() << std::endl;
        }
    }

    currentGameId = 0;
    selectedCarIndex = -1;

    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        refreshGamesList();
    }
}

void LobbyController::openWaitingRoom() {
    waitingRoomWindow = new WaitingRoomWindow(8);

    for (const auto& username : pendingPlayers) {
        waitingRoomWindow->addPlayerByName(username);
        auto it = pendingCars.find(username);
        if (it != pendingCars.end()) waitingRoomWindow->setPlayerCarByName(username, it->second);
    }
    pendingPlayers.clear();
    pendingCars.clear();

    waitingRoomWindow->addPlayerByName(playerName);

    connect(waitingRoomWindow, &WaitingRoomWindow::readyToggled, this,
            &LobbyController::onPlayerReadyToggled);
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested, this,
            &LobbyController::onStartGameRequested);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested, this,
            &LobbyController::onBackFromWaitingRoom);

    waitingRoomWindow->show();
}

void LobbyController::onPlayerReadyToggled(bool isReady) {
    try {
        if (waitingRoomWindow) waitingRoomWindow->setPlayerReadyByName(playerName, isReady);
        lobbyClient->set_ready(isReady);
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onStartGameRequested() {
    try {
        if (!lobbyClient) throw std::runtime_error("LobbyClient null");
        lobbyClient->start_game(currentGameId);
        if (lobbyClient) lobbyClient->stop_listening();
        finishLobby(true);
    } catch (const std::exception& e) {
        handleNetworkError(e);
        finishLobby(false);
    }
}

void LobbyController::onBackFromWaitingRoom() {
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }

    if (lobbyClient && currentGameId > 0) {
        try {
            lobbyClient->leave_game(currentGameId);
        } catch (const std::exception& e) {}
    }

    if (lobbyClient) lobbyClient->stop_listening(false);

    currentGameId = 0;
    selectedCarIndex = -1;
    pendingPlayers.clear();
    pendingCars.clear();

    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        refreshGamesList();
    } else {
        openMatchSelection();
    }
}