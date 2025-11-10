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
    
    // Crear y mostrar ventana principal del lobby
    lobbyWindow = new LobbyWindow();
    
    // Conectar señal del botón "Jugar"
    connect(lobbyWindow, &LobbyWindow::playRequested,
            this, &LobbyController::onPlayClicked);
    
    // Mostrar ventana
    lobbyWindow->show();
}

void LobbyController::onPlayClicked() {
    std::cout << "[Controller] Usuario presionó 'Jugar'" << std::endl;
    
    // Ocultar lobby principal
    lobbyWindow->hide();
    
    try {
        // Conectar al servidor AHORA (no antes)
        connectToServer();
        
        // Mostrar ventana de ingreso de nombre
        std::cout << "[Controller] Mostrando ventana de ingreso de nombre" << std::endl;
        nameInputWindow = new NameInputWindow();
        
        // Conectar señales
        connect(nameInputWindow, &NameInputWindow::nameConfirmed,
                this, &LobbyController::onNameConfirmed);
        
        nameInputWindow->show();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
        // Volver a mostrar el lobby principal si falla
        lobbyWindow->show();
    }
}

void LobbyController::connectToServer() {
    std::cout << "[Controller] Conectando al servidor " 
              << serverHost.toStdString() << ":" << serverPort.toStdString() 
              << "..." << std::endl;
    
    // Crear conexión con el servidor
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
        //Enviar nombre al servidor
        lobbyClient->send_username(name.toStdString());
        std::cout << "[Controller] Nombre enviado al servidor" << std::endl;
        
        //Esperar mensaje de bienvenida
        std::cout << "[Controller] Esperando mensaje de bienvenida..." << std::endl;
        std::string welcome = lobbyClient->receive_welcome();
        
        //Mostrar mensaje de bienvenida
        std::cout << "[Controller] Bienvenida recibida: " << welcome << std::endl;
        
        //Cerrar ventana de nombre
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
        
        //Abrir ventana de selección de partidas
        openMatchSelection();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onBackFromNameInput() {
    std::cout << "[Controller] Usuario presionó 'Volver' desde ingreso de nombre" << std::endl;
    
    // Cerrar ventana de nombre
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    
    // Desconectar del servidor
    lobbyClient.reset();
    std::cout << "[Controller] Desconectado del servidor" << std::endl;
    
    // Volver a mostrar lobby principal
    lobbyWindow->show();
}

void LobbyController::handleNetworkError(const std::exception& e) {
    std::cerr << "[Controller]  Error: " << e.what() << std::endl;
    
    // Determinar ventana actual
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
    
    // Detectar desconexión
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
    
    // Resetear estado
    lobbyClient.reset();
    currentGameId = 0;
    selectedCarIndex = -1;
    playerName.clear();
    
    std::cout << "[Controller] Estado limpiado" << std::endl;
    
    // Volver al lobby principal
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
        // Enviar petición al servidor para crear la partida
        lobbyClient->create_game(matchName.toStdString(), static_cast<uint8_t>(maxPlayers));
        
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
    
    // Cerrar ventana de creación
    if (createMatchWindow) {
        createMatchWindow->close();
        createMatchWindow->deleteLater();
        createMatchWindow = nullptr;
    }
    
    // Volver a mostrar selección de partidas
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
    }
}

void LobbyController::openMatchSelection() {
    std::cout << "[Controller] Abriendo ventana de selección de partidas" << std::endl;
    
    // Crear ventana de selección de partidas
    matchSelectionWindow = new MatchSelectionWindow();
    
    // Conectar señales
    connect(matchSelectionWindow, &MatchSelectionWindow::joinMatchRequested,
            this, &LobbyController::onJoinMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::createMatchRequested,
            this, &LobbyController::onCreateMatchRequested);
    connect(matchSelectionWindow, &MatchSelectionWindow::backToLobby,
            this, &LobbyController::onBackFromMatchSelection);
    connect(matchSelectionWindow, &MatchSelectionWindow::refreshRequested,
            this, &LobbyController::onRefreshMatchList);
    
    // Mostrar ventana
    matchSelectionWindow->show();
    
    // Cargar lista de partidas
    refreshGamesList();
}

void LobbyController::refreshGamesList() {
    std::cout << "[Controller] Solicitando lista de partidas al servidor..." << std::endl;
    
    try {
        //Pedir lista al servidor
        lobbyClient->request_games_list();
        
        //Recibir lista
        std::vector<GameInfo> games = lobbyClient->receive_games_list();
        
        std::cout << "[Controller] Recibidas " << games.size() << " partidas" << std::endl;
        
        //Actualizar la vista con los datos reales
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
    
    // Obtener el item seleccionado de la lista
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
        //Enviar petición al servidor
        lobbyClient->join_game(gameId);
        
        //Recibir confirmación
        uint16_t confirmedGameId = lobbyClient->receive_game_joined();
        
        //Guardar ID de la partida actual
        currentGameId = confirmedGameId;
        
        std::cout << "[Controller] Unido exitosamente a partida ID: " 
                  << currentGameId << std::endl;
   
        //Ocultar ventana de selección
        if (matchSelectionWindow) {
            matchSelectionWindow->hide();
        }
        
        //Abrir garage
        std::cout << "[Controller] Abriendo garage..." << std::endl;
        openGarage();
        
    } catch (const std::exception& e) {
        handleNetworkError(e);
    }
}

void LobbyController::onCreateMatchRequested() {
    std::cout << "[Controller] Usuario quiere crear nueva partida" << std::endl;
    
    // Ocultar ventana de selección de partidas
    if (matchSelectionWindow) {
        matchSelectionWindow->hide();
    }
    
    // Crear ventana de creación de partida
    createMatchWindow = new CreateMatchWindow();
    
    // Conectar señales
    connect(createMatchWindow, &CreateMatchWindow::matchCreated,
            this, &LobbyController::onMatchCreated);
    connect(createMatchWindow, &CreateMatchWindow::backRequested,
            this, &LobbyController::onBackFromCreateMatch);
    
    // Mostrar ventana
    createMatchWindow->show();
}

void LobbyController::onBackFromMatchSelection() {
    std::cout << "[Controller] Volviendo al lobby principal desde match selection" << std::endl;
    
    // Cerrar ventana de selección
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        matchSelectionWindow->deleteLater();
        matchSelectionWindow = nullptr;
    }
    
    // Desconectar del servidor
    lobbyClient.reset();
    std::cout << "[Controller] Desconectado del servidor" << std::endl;
    
    // Volver a mostrar lobby principal
    if (lobbyWindow) {
        lobbyWindow->show();
    }
}

void LobbyController::openGarage() {
    std::cout << "[Controller] Abriendo ventana de garage..." << std::endl;
    
    // Crear ventana de garage
    garageWindow = new GarageWindow();
    
    // Conectar señales
    connect(garageWindow, &GarageWindow::carSelected,
            this, &LobbyController::onCarSelected);
    connect(garageWindow, &GarageWindow::backRequested,
            this, &LobbyController::onBackFromGarage);
    
    // Mostrar ventana
    garageWindow->show();
}

void LobbyController::onCarSelected(int carIndex) {
    std::cout << "[Controller] Auto seleccionado: índice " << carIndex << std::endl;
    
    selectedCarIndex = carIndex;
    
    // No enviar al servidor todavía, falta la implementación del lado del server
   // lobbyClient->select_car(static_cast<uint8_t>(carIndex));
    
    // Cerrar ventana de garage
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }
    
    // Abrir sala de espera
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    openWaitingRoom();
}

void LobbyController::onBackFromGarage() {
    std::cout << "[Controller] Usuario volvió desde garage" << std::endl;
    
    // Cerrar ventana de garage
    if (garageWindow) {
        garageWindow->close();
        garageWindow->deleteLater();
        garageWindow = nullptr;
    }
    
    // No enviar al servidor todavía, falta la implementación del lado del server
    //lobbyClient->leave_game(currentGameId);
    
    // Volver a mostrar match selection
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        // Refrescar lista
        refreshGamesList();
    }
}

void LobbyController::openWaitingRoom() {
    std::cout << "[Controller] Abriendo sala de espera..." << std::endl;
    
    // Crear ventana de sala de espera
    waitingRoomWindow = new WaitingRoomWindow();
    
    // Nombres de los autos para mostrar
    const std::vector<QString> carNames = {
        "Leyenda Urbana", "Brisa", "J-Classic 600", "Cavallo V8", 
        "Senator", "Nómada", "Stallion GT"
    };
    
    // Obtener nombre del auto seleccionado
    QString carName = "Auto Desconocido";
    if (selectedCarIndex >= 0 && selectedCarIndex < static_cast<int>(carNames.size())) {
        carName = carNames[selectedCarIndex];
    }
    
    // Configurar información del jugador local
    waitingRoomWindow->setLocalPlayerInfo(playerName, carName);
    
    // Conectar señales
    connect(waitingRoomWindow, &WaitingRoomWindow::readyToggled,
            this, &LobbyController::onPlayerReadyToggled);
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested,
            this, &LobbyController::onStartGameRequested);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested,
            this, &LobbyController::onBackFromWaitingRoom);
    
    // Mostrar ventana
    waitingRoomWindow->show();
}

void LobbyController::onPlayerReadyToggled(bool isReady) {
    std::cout << "[Controller] Jugador marcado como: " 
              << (isReady ? "LISTO" : "NO LISTO") << std::endl;
    
    // No enviar al servidor todavía, falta la implementación del lado del server
    //lobbyClient->set_ready(isReady);
}

void LobbyController::onStartGameRequested() {
    std::cout << "[Controller] Solicitando inicio de partida..." << std::endl;
    
    // No enviar al servidor todavía, falta la implementación del lado del server
   // lobbyClient->start_game(currentGameId);
    
    
}

void LobbyController::onBackFromWaitingRoom() {
    std::cout << "[Controller] Usuario salió de la sala de espera" << std::endl;
    
    // Cerrar ventana de sala de espera
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        waitingRoomWindow->deleteLater();
        waitingRoomWindow = nullptr;
    }
    
    // No enviar al servidor todavía, falta la implementación del lado del server
   // lobbyClient->leave_game(currentGameId);
    
    // Volver a match selection
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
        refreshGamesList();
    }
}