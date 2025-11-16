#ifndef LOBBY_WINDOW_H
#define LOBBY_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <SDL2pp/Mixer.hh>
#include <vector>
#include "common_types.h"
#include "garage_window.h"
#include <cstdint>

// Forward declarations
class NameInputWindow;
class MatchSelectionWindow;
class GarageWindow;
class WaitingRoomWindow;
class CreateMatchWindow;

class LobbyWindow : public QWidget {
    Q_OBJECT

public:
    // Constructor sin parámetros - lobby inicial
    explicit LobbyWindow(QWidget *parent = nullptr);
    ~LobbyWindow();
    uint8_t _max_players;

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

signals:
    void playRequested();    

private slots:
    void onPlayClicked();
    void onNameConfirmed(const QString& playerName);
    void onJoinMatch(const QString& matchId);
    void onCreateMatch();
    void onCarSelected(const CarInfo& car);
    void onStartGame();
    void onBackFromNameInput();
    void onBackFromMatchSelection();
    void onBackFromGarage();
    void onBackFromWaitingRoom();
    void onMatchCreated(const QString& matchName, int maxPlayers, const std::vector<RaceConfig>& races);
    void onBackFromCreateMatch();

private:
    void initAudio();
    void loadMusic(const char* musicPath);
    void cleanupAudio();
    void openGarage();
    
    // Player info
    QString playerName;
    int selectedCarIndex;
    
    // UI Elements
    QPushButton* playButton;
    QPushButton* quitButton;
    QLabel* titleLabel;
    QPixmap backgroundImage;
    
    // Audio
    Mix_Music* backgroundMusic;
    bool audioInitialized;
    
    // Font
    int customFontId;
    
    // Ventanas secundarias
    NameInputWindow* nameInputWindow;
    MatchSelectionWindow* matchSelectionWindow;
    GarageWindow* garageWindow;
    WaitingRoomWindow* waitingRoomWindow;
    CreateMatchWindow* createMatchWindow;
};

#endif // LOBBY_WINDOW_H