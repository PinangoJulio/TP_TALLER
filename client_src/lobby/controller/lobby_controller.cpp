#include "lobby_controller.h"

LobbyController::LobbyController(const QString& host, const QString& port, QObject* parent)
    : QObject(parent), 
      serverHost(host),
      serverPort(port),
      lobbyWindow(nullptr),
      nameInputWindow(nullptr),
      matchSelectionWindow(nullptr),
      createMatchWindow(nullptr),
      garageWindow(nullptr),
      waitingRoomWindow(nullptr),
      currentGameId(0),
      selectedCarIndex(-1) {
    
    std::cout << "[Controller] Controlador creado (sin conectar al servidor todavía)" << std::endl;
    std::cout << "[Controller] Servidor configurado: " 
              << host.toStdString() << ":" << port.toStdString() << std::endl;
}

LobbyController::~LobbyController() {
    std::cout << "[Controller] Destructor llamado" << std::endl;
}

void LobbyController::start() {
    std::cout << "[Controller] Mostrando lobby principal" << std::endl;
    
    lobbyWindow = new LobbyWindow();
    
    connect(lobbyWindow, &LobbyWindow::playRequested,
            this, &LobbyController::onPlayClicked);
    
    lobbyWindow->show();
}

void LobbyController::onPlayClicked() {
    std::cout << "[Controller] Usuario presionó 'Jugar'" << std::endl;
    
    lobbyWindow->hide();
    
    try {
        connectToServer();
        
        std::cout << "[Controller] Mostrando ventana de ingreso de nombre" << std::endl;
        nameInputWindow = new NameInputWindow();
        
        connect(nameInputWindow, &NameInputWindow::nameConfirmed,
                this, &LobbyController::onNameConfirmed);
        
        nameInputWindow->show();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
        lobbyWindow->show();
    }
}

void LobbyController::connectToServer() {
    std::cout << "[Controller] Conectando al servidor " 
              << serverHost.toStdString() << ":" << serverPort.toStdString() 
              << "..." << std::endl;
    
    lobbyClient = std::make_unique<LobbyClient>(
        serverHost.toStdString(), 
        serverPort.toStdString()
    );
    
    std::cout << "[Controller] Conectado exitosamente" << std::endl;
}

void LobbyController::onNameConfirmed(const QString& name) {
    std::cout << "[Controller] Usuario confirmó nombre: " 
              << name.toStdString() << std::endl;
    
    playerName = name;
    
    try {
        lobbyClient->send_username(name.toStdString());
        std::cout << "[Controller] Nombre enviado al servidor" << std::endl;
        
        std::cout << "[Controller] Esperando mensaje de bienvenida..." << std::endl;
        std::string welcome = lobbyClient->receive_welcome();
        
        std::cout << "[Controller] Bienvenida recibida: " << welcome << std::endl;
        
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
        
        openMatchSelection();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromNameInput() {
    std::cout << "[Controller] Usuario presionó 'Volver' desde ingreso de nombre" << std::endl;
    
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    
    lobbyClient.reset();
    std::cout << "[Controller] Desconectado del servidor" << std::endl;
    
    lobbyWindow->show();
}

void LobbyController::handleNetworkError(const std::exception& e) {
    std::cerr << "[Controller] ❌ Error: " << e.what() << std::endl;
    
    QWidget* currentWindow = lobbyWindow;
    
    if (waitingRoomWindow && waitingRoomWindow->isVisible()) {
        currentWindow = waitingRoomWindow;
    } else if (garageWindow && garageWindow->isVisible()) {
        currentWindow = garageWindow;
    } else if (createMatchWindow && createMatchWindow->isVisible()) {
        currentWindow = createMatchWindow;
    } else if (matchSelectionWindow && matchSelectionWindow->isVisible()) {
        currentWindow = matchSelectionWindow;
    } else if (nameInputWindow && nameInputWindow->isVisible()) {
        currentWindow = nameInputWindow;
    }
    
    QString errorMsg = QString::fromStdString(e.what());
    
    if (errorMsg.contains("Connection closed") || 
        errorMsg.contains("closed by server") ||
        errorMsg.contains("EOF")) {
        
        QMessageBox::critical(currentWindow,
            "Desconectado",
            "El servidor cerró la conexión.\n\n"
            "Posibles causas:\n"
            "  • Timeout de conexión\n"
            "  • Error de protocolo\n\n"
            "Volviendo al lobby principal...");
        
        cleanupAndReturnToLobby();
        
    } else {
        QMessageBox::critical(currentWindow,
            "Error de Red",
            QString("Error de comunicación:\n\n%1\n\n"
                    "Verifica que el servidor esté corriendo.").arg(errorMsg));
    }
}

void LobbyController::cleanupAndReturnToLobby() {
    std::cout << "[Controller] Limpiando estado y volviendo al lobby..." << std::endl;
    
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
    if (createMatchWindow) {
        createMatchWindow->close();
        createMatchWindow->deleteLater();
        createMatchWindow = nullptr;
    }
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        matchSelectionWindow->deleteLater();
        matchSelectionWindow = nullptr;
    }
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    
    lobbyClient.reset();
    currentGameId = 0;
    selectedCarIndex = -1;
    playerName.clear();
    
    std::cout << "[Controller] Estado limpiado" << std::endl;
    
    if (lobbyWindow) {
        lobbyWindow->show();
    }
}

void LobbyController::onMatchCreated(const QString& matchName, int maxPlayers, const std::vector<RaceConfig>& races) {
    std::cout << "[Controller] Usuario confirmó creación de partida:" << std::endl;
    std::cout << "  Nombre: " << matchName.toStdString() << std::endl;
    std::cout << "  Jugadores máximos: " << maxPlayers << std::endl;
    std::cout << "  Número de carreras: " << races.size() << std::endl;
    
    std::vector<std::pair<std::string, std::string>> race_pairs;
    race_pairs.reserve(races.size());
    
    for (const auto& race : races) {
        std::cout << "  Carrera: "
                  << race.cityName.toStdString()
                  << " - " << race.trackName.toStdString() << std::endl;

        race_pairs.emplace_back(race.cityName.toStdString(), race.trackName.toStdString());
    }

    try {
        lobbyClient->create_game(
            matchName.toStdString(), 
            static_cast<uint8_t>(maxPlayers), 
            race_pairs
        );
        
        currentGameId = lobbyClient->receive_game_created();
        std::cout << "[Controller] Partida creada con ID: " << currentGameId << std::endl;
        
        if (createMatchWindow) {
            createMatchWindow->close();
            createMatchWindow->deleteLater();
            createMatchWindow = nullptr;
        }
        
        // Conectar señales pero NO iniciar listener todavía
        if (!lobbyClient->is_listening()) {
            std::cout << "[Controller] Conectando señales de notificaciones..." << std::endl;
            connectNotificationSignals();
        }
        
        std::cout << "[Controller] Abriendo garage..." << std::endl;
        openGarage();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromCreateMatch() {
    std::cout << "[Controller] Usuario canceló creación de partida" << std::endl;
    
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
    std::cout << "[Controller] Abriendo ventana de selección de partidas" << std::endl;
    
    matchSelectionWindow = new MatchSelectionWindow();
    
    connect(matchSelectionWindow, &MatchSelectionWindow::joinMatchRequested,
            this, &LobbyController::onJoinMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::createMatchRequested,
            this, &LobbyController::onCreateMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::backToLobby,
            this, &LobbyController::onBackFromMatchSelection);
    connect(matchSelectionWindow, &MatchSelectionWindow::refreshRequested,
            this, &LobbyController::onRefreshMatchList);
    
    matchSelectionWindow->show();
    
    refreshGamesList();
}

void LobbyController::refreshGamesList() {
    std::cout << "[Controller] Solicitando lista de partidas al servidor..." << std::endl;
    
    try {
        lobbyClient->request_games_list();
        
        std::vector<GameInfo> games = lobbyClient->receive_games_list();
        
        std::cout << "[Controller] Recibidas " << games.size() << " partidas" << std::endl;
        
        if (matchSelectionWindow) {
            matchSelectionWindow->updateGamesList(games);
        }
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onRefreshMatchList() {
    std::cout << "[Controller] Usuario solicitó actualizar lista" << std::endl;
    refreshGamesList();
}

void LobbyController::onJoinMatchRequested(const QString& matchId) {
    std::cout << "[Controller] Usuario quiere unirse a partida con matchId: " 
              << matchId.toStdString() << std::endl;
    
    if (!matchSelectionWindow) {
        std::cerr << "[Controller] Error: matchSelectionWindow es nullptr" << std::endl;
        return;
    }
    
    QListWidgetItem* selectedItem = matchSelectionWindow->getSelectedItem();
    if (!selectedItem) {
        QMessageBox::warning(matchSelectionWindow,
            "Selección requerida",
            "Por favor selecciona una partida de la lista");
        return;
    }
    
    uint16_t gameId = selectedItem->data(Qt::UserRole).toUInt();
    
    std::cout << "[Controller] Intentando unirse a game_id (de UserRole): " 
              << gameId << std::endl;
    
    try {
        lobbyClient->join_game(gameId);
        
        uint16_t confirmedGameId = lobbyClient->receive_game_joined();
        
        currentGameId = confirmedGameId;
        
        std::cout << "[Controller] Unido exitosamente a partida ID: " 
                  << currentGameId << std::endl;
        
        std::vector<QString> snapshotPlayers;
        std::map<QString, QString> snapshotCars;
        
        lobbyClient->read_room_snapshot(snapshotPlayers, snapshotCars);
        
        pendingPlayers = snapshotPlayers;
        pendingCars = snapshotCars;
        
        std::cout << "[Controller] Snapshot recibido: " << pendingPlayers.size() << " jugadores" << std::endl;
        
        if (matchSelectionWindow) {
            matchSelectionWindow->hide();
        }
        
        if (!lobbyClient->is_listening()) {
            std::cout << "[Controller] Conectando señales de notificaciones..." << std::endl;
            connectNotificationSignals();
        }
        
        std::cout << "[Controller] Abriendo garage..." << std::endl;
        openGarage();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onCreateMatchRequested() {
    std::cout << "[Controller] Usuario quiere crear nueva partida" << std::endl;
    
    if (matchSelectionWindow) {
        matchSelectionWindow->hide();
    }
    
    createMatchWindow = new CreateMatchWindow();
    
    connect(createMatchWindow, &CreateMatchWindow::matchCreated,
            this, &LobbyController::onMatchCreated);
    connect(createMatchWindow, &CreateMatchWindow::backRequested,
            this, &LobbyController::onBackFromCreateMatch);
    
    createMatchWindow->show();
}

void LobbyController::onBackFromMatchSelection() {
    std::cout << "[Controller] Volviendo al lobby principal desde match selection" << std::endl;
    
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        matchSelectionWindow->deleteLater();
        matchSelectionWindow = nullptr;
    }
    
    lobbyClient.reset();
    std::cout << "[Controller] Desconectado del servidor" << std::endl;
    
    if (lobbyWindow) {
        lobbyWindow->show();
    }
}

void LobbyController::openGarage() {
    std::cout << "[Controller] Abriendo ventana de garage..." << std::endl;
    
    garageWindow = new GarageWindow();
    
    connect(garageWindow, &GarageWindow::carSelected,
            this, &LobbyController::onCarSelected);
    connect(garageWindow, &GarageWindow::backRequested,
            this, &LobbyController::onBackFromGarage);
    
    garageWindow->show();
}

void LobbyController::onCarSelected(const CarInfo& car) {
    std::cout << "[Controller] Auto seleccionado: " << car.name.toStdString() << std::endl;

    try {
        std::cout << "[Controller] DEBUG: Enviando selección de auto..." << std::endl;
        lobbyClient->select_car(car.name.toStdString(), car.type.toStdString());
        
        std::cout << "[Controller] DEBUG: Esperando ACK..." << std::endl;
        std::string car_confirmed = lobbyClient->receive_car_confirmation();
        std::cout << "[Controller] ✅ CAR confirmed: " << car_confirmed << std::endl;

        if (garageWindow) {
            garageWindow->close();
            garageWindow->deleteLater();
            garageWindow = nullptr;
        }
        
        if (!lobbyClient->is_listening()) {
            std::cout << "[Controller] 🚀 Starting listener NOW..." << std::endl;
            lobbyClient->start_listening();
        }
        
        std::cout << "[Controller] Listener status: " 
                  << (lobbyClient->is_listening() ? "RUNNING" : "STOPPED") << std::endl;
        
        std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
        openWaitingRoom();
        
        if (waitingRoomWindow) {
            std::cout << "[Controller] Actualizando auto del jugador local: " 
                      << car.name.toStdString() << std::endl;
            waitingRoomWindow->setPlayerCarByName(playerName, car.name);
        }
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::connectNotificationSignals() {
    // Conectar señales de notificaciones
    connect(lobbyClient.get(), &LobbyClient::playerJoinedNotification,
            this, [this](QString username) {
                std::cout << "[Controller] Notification: Player joined: " 
                          << username.toStdString() << std::endl;
                
                if (waitingRoomWindow) {
                    waitingRoomWindow->addPlayerByName(username);
                } else {
                    pendingPlayers.push_back(username);
                    std::cout << "[Controller] Stored pending player: " << username.toStdString() << std::endl;
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::playerLeftNotification,
            this, [this](QString username) {
                std::cout << "[Controller] Notification: Player left: " 
                          << username.toStdString() << std::endl;
                if (waitingRoomWindow) {
                    waitingRoomWindow->removePlayerByName(username);
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::playerReadyNotification,
            this, [this](QString username, bool isReady) {
                std::cout << "[Controller] 🔔 Notification: Player " << username.toStdString() 
                          << " ready: " << isReady 
                          << " (local player: " << playerName.toStdString() << ")" << std::endl;
                
                //Verificar si es el jugador local
                if (username == playerName) {
                    std::cout << "[Controller] ⚠️  WARNING: Received ready notification for LOCAL player!" << std::endl;
                }
                
                if (waitingRoomWindow) {
                    waitingRoomWindow->setPlayerReadyByName(username, isReady);
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::carSelectedNotification,
            this, [this](QString username, QString carName, QString) {
                std::cout << "[Controller] Notification: Player " << username.toStdString() 
                          << " selected " << carName.toStdString() << std::endl;
                
                if (waitingRoomWindow) {
                    waitingRoomWindow->setPlayerCarByName(username, carName);
                } else {
                    pendingCars[username] = carName;
                    std::cout << "[Controller] Stored pending car for " << username.toStdString() << std::endl;
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::gameStartedNotification,
            this, [this]() {
                std::cout << "[Controller] Notification: Game starting!" << std::endl;
                if (waitingRoomWindow) {
                    waitingRoomWindow->close();
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::errorOccurred,
            this, [this](QString errorMsg) {
                std::cerr << "[Controller] Error: " << errorMsg.toStdString() << std::endl;
                QMessageBox::critical(waitingRoomWindow, "Error", errorMsg);
            });

    connect(lobbyClient.get(), &LobbyClient::gamesListReceived,
            this, [this](std::vector<GameInfo> games) {
                std::cout << "[Controller] Received games list update (" << games.size() << " games)" << std::endl;
                if (matchSelectionWindow) {
                    matchSelectionWindow->updateGamesList(games);
                }
            });
}

void LobbyController::onBackFromGarage() {
    std::cout << "[Controller] Usuario volvió desde garage" << std::endl;
    
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }

    if (lobbyClient && currentGameId > 0) {
        try {
            std::cout << "[Controller] Enviando leave_game para partida " << currentGameId << std::endl;
            lobbyClient->leave_game(currentGameId);
            std::cout << "[Controller] ✅ Leave confirmado" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[Controller] ⚠️ Error al abandonar partida: " << e.what() << std::endl;
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
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    
    uint8_t maxPlayers = 8;
    waitingRoomWindow = new WaitingRoomWindow(maxPlayers);
    
    // 1. Procesar jugadores pendientes (del snapshot)
    for (const auto& username : pendingPlayers) {
        std::cout << "[Controller] Procesando jugador pendiente: " << username.toStdString() << std::endl;
        waitingRoomWindow->addPlayerByName(username);
        
        auto it = pendingCars.find(username);
        if (it != pendingCars.end()) {
            std::cout << "[Controller] Procesando auto pendiente: " << it->second.toStdString() << std::endl;
            waitingRoomWindow->setPlayerCarByName(username, it->second);
        }
    }

    // Limpiar pendientes
    pendingPlayers.clear();
    pendingCars.clear();
    
    // 2. Agregar jugador local
    std::cout << "[Controller] Adding local player: " << playerName.toStdString() << std::endl;
    waitingRoomWindow->addPlayerByName(playerName);
    
    // 3. Conectar botones de la UI
    connect(waitingRoomWindow, &WaitingRoomWindow::readyToggled,
            this, &LobbyController::onPlayerReadyToggled);
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested,
            this, &LobbyController::onStartGameRequested);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested,
            this, &LobbyController::onBackFromWaitingRoom);
    
    waitingRoomWindow->show();
    
    std::cout << "[Controller] Sala de espera inicializada" << std::endl;
}

void LobbyController::onPlayerReadyToggled(bool isReady) {
    std::cout << "[Controller] Jugador marcado como: " 
              << (isReady ? "LISTO" : "NO LISTO") << std::endl;
    
    try {
        // PRIMERO marcar localmente
        if (waitingRoomWindow) {
            waitingRoomWindow->setPlayerReadyByName(playerName, isReady);
            std::cout << "[Controller] ✅ Local ready state updated for " << playerName.toStdString() << std::endl;
        }
        
        // Luego enviar al servidor
        lobbyClient->set_ready(isReady);
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onStartGameRequested() {
    std::cout << "[Controller] Solicitando inicio de partida..." << std::endl;
    
    try {
        lobbyClient->start_game(currentGameId);
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromWaitingRoom() {
    std::cout << "[Controller] Usuario salió de la sala de espera" << std::endl;
    
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }
    
    if (lobbyClient && currentGameId > 0) {
        try {
            std::cout << "[Controller] Enviando leave_game para partida " << currentGameId << std::endl;
            lobbyClient->leave_game(currentGameId);
            std::cout << "[Controller] Leave game enviado" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[Controller] ⚠️ Error al enviar leave_game: " << e.what() << std::endl;
        }
    }
    
    if (lobbyClient) {
        std::cout << "[Controller] Esperando a que el listener se detenga..." << std::endl;
        
        int timeout = 30;
        while (lobbyClient->is_listening() && timeout > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            timeout--;
        }
        
        if (timeout == 0) {
            std::cerr << "[Controller] ⚠️ Timeout esperando listener" << std::endl;
        }
        
        lobbyClient->stop_listening();
        std::cout << "[Controller] Listener detenido completamente" << std::endl;
    }
    
    currentGameId = 0;
    selectedCarIndex = -1;
    
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
    }
    
    std::cout << "[Controller] Flujo de salida completado" << std::endl;
}