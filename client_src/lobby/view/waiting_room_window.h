#ifndef WAITING_ROOM_WINDOW_H
#define WAITING_ROOM_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QTimer>
#include <vector>
#include "base_lobby.h"

struct PlayerInfo {
    QString name;
    QString carName;
    bool isReady;
    bool isLocal;
};

struct PlayerCardWidgets {
    QWidget* container;
    QWidget* statusCircle;
    QLabel* nameLabel;
    QLabel* carLabel;
    QLabel* statusLabel;
};

class WaitingRoomWindow : public BaseLobby{
    Q_OBJECT

public:
    explicit WaitingRoomWindow(QWidget *parent = nullptr);
    ~WaitingRoomWindow();
    
    void addPlayer(const QString& name, const QString& car, bool isLocal = false);
    void setPlayerReady(int playerIndex, bool ready);
    void setLocalPlayerInfo(const QString& name, const QString& car);

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void readyToggled(bool isReady);
    void startGameRequested();
    void backRequested();

private slots:
    void onReadyClicked();
    void onStartClicked();
    void onBackClicked();
    void updateReadyAnimation();
    void onPreviousPage();
    void onNextPage();

private:
    void setupUI();
    void updatePlayerDisplay();
    void createPlayerCards();
    void updatePaginationButtons();
    
    QPixmap backgroundImage;
    
    std::vector<PlayerInfo> players;
    std::vector<PlayerCardWidgets> playerCardWidgets;
    bool localPlayerReady;
    int currentPage;
    
    QLabel* titleLabel;
    QLabel* statusLabel;
    QLabel* pageLabel;
    QWidget* playersPanel;
    
    QPushButton* readyButton;
    QPushButton* startButton;
    QPushButton* backButton;
    QPushButton* prevPageButton;
    QPushButton* nextPageButton;
    
    QTimer* animationTimer;
    int animationFrame;
    
    int customFontId;
};

#endif // WAITING_ROOM_WINDOW_H