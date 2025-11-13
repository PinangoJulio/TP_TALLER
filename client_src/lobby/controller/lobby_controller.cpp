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
    
    for (size_t i = 0; i < races.size(); i++) {
        std::cout << "  Carrera " << (i+1) << ": " 
                  << races[i].cityName.toStdString() 
                  << " - " << races[i].trackName.toStdString() << std::endl;
    }
    
    try {
        lobbyClient->create_game(matchName.toStdString(), static_cast<uint8_t>(maxPlayers));
        
        currentGameId = lobbyClient->receive_game_created();
        
        std::cout << "[Controller] Partida creada con ID: " << currentGameId << std::endl;
        
        if (createMatchWindow) {
            createMatchWindow->close();
            createMatchWindow->deleteLater();
            createMatchWindow = nullptr;
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

void LobbyController::onCarSelected(int carIndex) {
    std::cout << "[Controller] Auto seleccionado: índice " << carIndex << std::endl;
    
    selectedCarIndex = carIndex;
    
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }
    
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    openWaitingRoom();
}

void LobbyController::onBackFromGarage() {
    std::cout << "[Controller] Usuario volvió desde garage" << std::endl;
    
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }
    
    // 🔥 AGREGADO: Abandonar la partida en el servidor
    try {
        lobbyClient->leave_game(currentGameId);
        std::cout << "[Controller] Abandonó la partida " << currentGameId << std::endl;
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

void LobbyController::openWaitingRoom() {
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    
    waitingRoomWindow = new WaitingRoomWindow();
    
    const std::vector<QString> carNames = {
        "Leyenda Urbana", "Brisa", "J-Classic 600", "Cavallo V8", 
        "Senator", "Nómada", "Stallion GT"
    };
    
    QString carName = "Auto Desconocido";
    if (selectedCarIndex >= 0 && selectedCarIndex < static_cast<int>(carNames.size())) {
        carName = carNames[selectedCarIndex];
    }
    
    waitingRoomWindow->setLocalPlayerInfo(playerName, carName);
    
    connect(waitingRoomWindow, &WaitingRoomWindow::readyToggled,
            this, &LobbyController::onPlayerReadyToggled);
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested,
            this, &LobbyController::onStartGameRequested);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested,
            this, &LobbyController::onBackFromWaitingRoom);
    
    waitingRoomWindow->show();
}

void LobbyController::onPlayerReadyToggled(bool isReady) {
    std::cout << "[Controller] Jugador marcado como: " 
              << (isReady ? "LISTO" : "NO LISTO") << std::endl;
}

void LobbyController::onStartGameRequested() {
    std::cout << "[Controller] Solicitando inicio de partida..." << std::endl;
}

void LobbyController::onBackFromWaitingRoom() {
    std::cout << "[Controller] Usuario salió de la sala de espera" << std::endl;
    
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }
    
    // 🔥 AGREGADO: Abandonar la partida en el servidor
    try {
        lobbyClient->leave_game(currentGameId);
        std::cout << "[Controller] Abandonó la partida " << currentGameId << std::endl;
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