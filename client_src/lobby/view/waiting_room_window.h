#ifndef WAITING_ROOM_WINDOW_H
#define WAITING_ROOM_WINDOW_H

#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <map>
#include <vector>

#include "base_lobby.h"

// Estructura específica para la UI de Qt (no confundir con PlayerInfoLobby de game_state.h)
struct PlayerCardData {
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

class WaitingRoomWindow : public BaseLobby {
    Q_OBJECT

public:
    explicit WaitingRoomWindow(uint8_t maxPlayers, QWidget* parent = nullptr);
    ~WaitingRoomWindow();

    // Métodos orientados a actualización por nombre
    void addPlayerByName(const QString& name);
    void removePlayerByName(const QString& name);
    void setPlayerReadyByName(const QString& name, bool ready);
    void setPlayerCarByName(const QString& name, const QString& car);

    // API previa por índice
    void addPlayer(const QString& name, const QString& car, bool isLocal = false);
    void setPlayerReady(int playerIndex, bool ready);
    void setLocalPlayerInfo(const QString& name, const QString& car);

    void updateStartButtonState();

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
    int getCardsPerPage() const;

    QPixmap backgroundImage;

    std::vector<PlayerCardData> players;
    std::map<QString, int> player_name_to_index;
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

    uint8_t max_players;
};

#endif  // WAITING_ROOM_WINDOW_H
