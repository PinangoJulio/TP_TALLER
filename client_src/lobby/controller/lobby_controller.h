#ifndef LOBBY_CONTROLLER_H
#define LOBBY_CONTROLLER_H

#include <QObject>
#include <QString>
#include <memory>


class LobbyClient;
class LobbyWindow;
class NameInputWindow;
class MatchSelectionWindow;

class LobbyController : public QObject {
    Q_OBJECT

private:
    // Configuración de conexión
    QString serverHost;
    QString serverPort;
    
   
    std::unique_ptr<LobbyClient> lobbyClient;
    
    // Vistas
    LobbyWindow* lobbyWindow;
    NameInputWindow* nameInputWindow;
    MatchSelectionWindow* matchSelectionWindow;
    
    // Estado
    QString playerName;

public:
    explicit LobbyController(const QString& host, const QString& port, QObject* parent = nullptr);
    ~LobbyController();
    
    // Iniciar el flujo (mostrar lobby principal)
    void start();

private slots:
    // usuario presiona Jugar en el lobby
    void onPlayClicked();
    
    // usuario confirma su nombre
    void onNameConfirmed(const QString& name);
    
    // usuario presiona Volver desde el nombre
    void onBackFromNameInput();
    
    // acciones en Match Selection
    void onRefreshMatchList();
    void onJoinMatchRequested(const QString& matchId);
    void onCreateMatchRequested();
    void onBackFromMatchSelection();

private:
    void connectToServer();
    void handleNetworkError(const std::exception& e);
    void openMatchSelection();
    void refreshGamesList();
};

#endif // LOBBY_CONTROLLER_H