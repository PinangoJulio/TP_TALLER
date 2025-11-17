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
    std::cerr << "[Controller]  Error: " << e.what() << std::endl;
    
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
    
    // ✅ Convertir RaceConfig (Qt) -> pair<string, string> (C++)
    std::vector<std::pair<std::string, std::string>> race_pairs;
    race_pairs.reserve(races.size());
    
    for (const auto& race : races) {
        std::cout << "  Carrera: "
                  << race.cityName.toStdString()
                  << " - " << race.trackName.toStdString() << std::endl;

        race_pairs.emplace_back(race.cityName.toStdString(), race.trackName.toStdString());
    }

    try {
        // ✅ Enviar con los 3 parámetros (nombre, max_players, carreras)
        lobbyClient->create_game(
            matchName.toStdString(), 
            static_cast<uint8_t>(maxPlayers), 
            race_pairs  // ← Este es el tercer parámetro que faltaba
        );
        
        // Recibir confirmación con el game_id
        currentGameId = lobbyClient->receive_game_created();
        std::cout << "[Controller] Partida creada con ID: " << currentGameId << std::endl;
        
        // Cerrar ventana de creación
        if (createMatchWindow) {
            createMatchWindow->close();
            createMatchWindow->deleteLater();
            createMatchWindow = nullptr;
        }
        
        // Abrir garage
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
        
        // 🔥 NUEVO: Registrar socket en el servidor para recibir broadcasts
        // (el servidor ya hace esto automáticamente en Receiver::handle_lobby cuando procesa MSG_JOIN_GAME)
        
        if (matchSelectionWindow) {
            matchSelectionWindow->hide();
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
    std::cout << "[Controller] Auto seleccionado: "
              << car.name.toStdString() << std::endl;

    try {
        lobbyClient->select_car(car.name.toStdString(), car.type.toStdString());
        std::string car_confirmed = lobbyClient->receive_car_confirmation();
        std::cout << "[Controller] CAR confirmed: " << car_confirmed << std::endl;

        if (garageWindow) {
            garageWindow->close();
            garageWindow->deleteLater();
            garageWindow = nullptr;
        }

        // 🔥 INICIAR LISTENER **ANTES** DE ABRIR WAITING ROOM
        std::cout << "[Controller] Iniciando listener de notificaciones..." << std::endl;
        lobbyClient->start_listening();
        
        std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
        openWaitingRoom();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}


void LobbyController::onBackFromGarage() {
    std::cout << "[Controller] Usuario volvió desde garage" << std::endl;
    
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;

    try {
        std::cout << "[Controller] Enviando leave_game para partida " << currentGameId << std::endl;
        lobbyClient->leave_game(currentGameId);

        auto games = lobbyClient->receive_games_list();
        std::cout << "[Controller] Leave confirmado. Juegos disponibles: " << games.size() << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[Controller] Error al abandonar partida: " << e.what() << std::endl;
    }
    
    // Resetear estado local
    currentGameId = 0;
    selectedCarIndex = -1;
    
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        refreshGamesList();
    }
}
}

void LobbyController::openWaitingRoom() {
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    
    uint8_t maxPlayers = 8;  // TODO: Obtener del servidor
    
    waitingRoomWindow = new WaitingRoomWindow(maxPlayers);
    
    // 🔥 DEFINIR carNames PRIMERO
    const std::vector<QString> carNames = {
        "Leyenda Urbana", "Brisa", "J-Classic 600", "Cavallo V8", 
        "Senator", "Nómada", "Stallion GT"
    };
    
    // 🔥 AGREGAR AL JUGADOR LOCAL
    waitingRoomWindow->addPlayerByName(playerName);
    
    // 🔥 CONFIGURAR SU AUTO (ahora carNames ya existe)
    QString carName = "Auto Desconocido";
    if (selectedCarIndex >= 0 && selectedCarIndex < static_cast<int>(carNames.size())) {
        carName = carNames[selectedCarIndex];
    }
    waitingRoomWindow->setPlayerCarByName(playerName, carName);
    
    // Conectar botones
    connect(waitingRoomWindow, &WaitingRoomWindow::readyToggled,
            this, &LobbyController::onPlayerReadyToggled);
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested,
            this, &LobbyController::onStartGameRequested);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested,
            this, &LobbyController::onBackFromWaitingRoom);

    connect(lobbyClient.get(), &LobbyClient::playerJoinedNotification,
            this, [this](QString username) {
                std::cout << "[Controller] Notification: Player joined: " 
                          << username.toStdString() << std::endl;
                if (waitingRoomWindow) {
                    waitingRoomWindow->addPlayerByName(username);
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
                std::cout << "[Controller] Notification: Player " << username.toStdString() 
                          << " ready: " << isReady << std::endl;
                if (waitingRoomWindow) {
                    waitingRoomWindow->setPlayerReadyByName(username, isReady);
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::carSelectedNotification,
            this, [this](QString username, QString carName) {
                std::cout << "[Controller] Notification: Player " << username.toStdString() 
                          << " selected " << carName.toStdString() << std::endl;
                if (waitingRoomWindow) {
                    waitingRoomWindow->setPlayerCarByName(username, carName);
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::gameStartedNotification,
            this, [this]() {
                std::cout << "[Controller] Notification: Game starting!" << std::endl;
                // TODO: Transición a SDL
                if (waitingRoomWindow) {
                    waitingRoomWindow->close();
                }
            });
    
    connect(lobbyClient.get(), &LobbyClient::errorOccurred,
            this, [this](QString errorMsg) {
                std::cerr << "[Controller] Error: " << errorMsg.toStdString() << std::endl;
                QMessageBox::critical(waitingRoomWindow, "Error", errorMsg);
            });
    
    waitingRoomWindow->show();
}

void LobbyController::onPlayerReadyToggled(bool isReady) {
    std::cout << "[Controller] Jugador marcado como: " 
              << (isReady ? "LISTO" : "NO LISTO") << std::endl;
    
    try {
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
    
    // 🔥 1. Cerrar ventana PRIMERO
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }
    
    // 🔥 2. Detener thread receptor
    if (lobbyClient) {
        lobbyClient->stop_listening();
    }

    // 🔥 3. Enviar leave_game (sin esperar respuesta)
    if (lobbyClient && currentGameId > 0) {
        try {
            std::cout << "[Controller] Enviando leave_game para partida " << currentGameId << std::endl;
            lobbyClient->leave_game(currentGameId);
            
            // 🔥 NO ESPERAR RESPUESTA - Volver inmediatamente
            std::cout << "[Controller] Leave game enviado, volviendo al match selection..." << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[Controller] Error al enviar leave_game: " << e.what() << std::endl;
        }
    }
    
    // 🔥 4. Resetear estado local
    currentGameId = 0;
    selectedCarIndex = -1;
    
    // 🔥 5. Recrear conexión (el socket quedó cerrado por stop_listening)
    try {
        std::cout << "[Controller] Recreando conexión al servidor..." << std::endl;
        
        lobbyClient.reset();
        connectToServer();
        lobbyClient->send_username(playerName.toStdString());
        lobbyClient->receive_welcome();
        
        std::cout << "[Controller] Reconexión exitosa" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[Controller] Error al reconectar: " << e.what() << std::endl;
        
        QMessageBox::warning(nullptr, 
            "Error de conexión",
            "No se pudo reconectar al servidor.\n"
            "Volviendo al lobby principal...");
        
        cleanupAndReturnToLobby();
        return;
    }
    
    // 🔥 6. Volver al match selection
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        refreshGamesList();
    } else {
        openMatchSelection();
    }
}