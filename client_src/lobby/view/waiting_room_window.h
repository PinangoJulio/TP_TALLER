#ifndef WAITING_ROOM_WINDOW_H
#define WAITING_ROOM_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QTimer>
#include <vector>
#include <map>  // ✅ NUEVO
#include <QMessageBox>

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

class WaitingRoomWindow : public QWidget {
    Q_OBJECT

public:
    explicit WaitingRoomWindow(uint8_t maxPlayers, QWidget *parent = nullptr);  // ✅ NUEVO: recibe maxPlayers
    ~WaitingRoomWindow();
    
    // 🔥 NUEVOS: Métodos para actualizar en vivo
    void addPlayerByName(const QString& name);
    void removePlayerByName(const QString& name);
    void setPlayerReadyByName(const QString& name, bool ready);
    void setPlayerCarByName(const QString& name, const QString& car);
    
    // Métodos existentes (compatible con código anterior)
    void addPlayer(const QString& name, const QString& car, bool isLocal = false);
    void setPlayerReady(int playerIndex, bool ready);
    void setLocalPlayerInfo(const QString& name, const QString& car);
    
    // 🔥 NUEVO: Habilitar/deshabilitar botón start según estado
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
    int getCardsPerPage() const;  // ✅ NUEVO: calcula tarjetas por página
    
    QPixmap backgroundImage;
    
    std::vector<PlayerInfo> players;
    std::map<QString, int> player_name_to_index;  // ✅ NUEVO: búsqueda rápida por nombre
    std::vector<PlayerCardWidgets> playerCardWidgets;
    bool localPlayerReady;
    int currentPage;
    uint8_t max_players;  // ✅ NUEVO: límite de jugadores
    
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