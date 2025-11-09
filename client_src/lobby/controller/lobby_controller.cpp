#include "lobby_controller.h"
#include "../model/lobby_client.h"
#include "../view/lobby_window.h"
#include "../view/name_input_window.h"
#include "../view/match_selection_window.h"
#include <QMessageBox>
#include <iostream>

LobbyController::LobbyController(const QString& host, const QString& port, QObject* parent)
    : QObject(parent), 
      serverHost(host),
      serverPort(port),
      lobbyWindow(nullptr),
      nameInputWindow(nullptr),
      matchSelectionWindow(nullptr) {
    
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
    
    // Conectar al boton jugar
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
        // Conectar al serviddor
        connectToServer();
        
        // Mostrar ventana de ingreso de nombre
        std::cout << "[Controller] Mostrando ventana de ingreso de nombre" << std::endl;
        nameInputWindow = new NameInputWindow();
        
        // Conectar
        connect(nameInputWindow, &NameInputWindow::nameConfirmed,
                this, &LobbyController::onNameConfirmed);
        
        
        // connect(nameInputWindow, &NameInputWindow::backRequested,
        //         this, &LobbyController::onBackFromNameInput);
        
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
        // Enviar nombre al servidor
        lobbyClient->send_username(name.toStdString());
        std::cout << "[Controller] Nombre enviado al servidor" << std::endl;
        
        // Esperar mensaje de bienvenida
        std::cout << "[Controller] Esperando mensaje de bienvenida..." << std::endl;
        std::string welcome = lobbyClient->receive_welcome();
        
        // Mostrar mensaje de bienvenida
        std::cout << "[Controller] Bienvenida recibida: " << welcome << std::endl;
        
       
        
        // Cerrar ventana de nombre
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
        
        // Abrir ventana de selección de partidas
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
    std::cerr << "[Controller]  Error de red: " << e.what() << std::endl;
    
    QWidget* parent = nameInputWindow ? static_cast<QWidget*>(nameInputWindow) 
                                      : static_cast<QWidget*>(lobbyWindow);
    
    QMessageBox::critical(
        parent, 
        "Error de Conexión",
        QString("Error al comunicarse con el servidor:\n%1").arg(e.what())
    );
    
    // Cerrar ventana de nombre si existe
    if (nameInputWindow) {
        nameInputWindow->close();
        nameInputWindow->deleteLater();
        nameInputWindow = nullptr;
    }
    
    // Volver a mostrar lobby principal
    if (lobbyWindow) {
        lobbyWindow->show();
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
        // Pedir lista al servidor
        lobbyClient->request_games_list();
        
        // Recibir lista
        std::vector<GameInfo> games = lobbyClient->receive_games_list();
        
        std::cout << "[Controller] Recibidas " << games.size() << " partidas" << std::endl;
        
        // Actualizar la vista con los datos reales
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
    std::cout << "[Controller] Usuario quiere unirse a partida: " 
              << matchId.toStdString() << std::endl;
    
    // TODO: Implementar lógica de unirse a partida
    QMessageBox::information(matchSelectionWindow, 
        "En desarrollo",
        "Función de unirse a partida aún no implementada");
}

void LobbyController::onCreateMatchRequested() {
    std::cout << "[Controller] Usuario quiere crear nueva partida" << std::endl;
    
    // TODO: Implementar lógica de crear partida
    QMessageBox::information(matchSelectionWindow, 
        "En desarrollo",
        "Función de crear partida aún no implementada");
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