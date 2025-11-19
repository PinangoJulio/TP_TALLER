#include "waiting_room_window.h"
#include <iostream>
#include <QPainter>
#include <QFontDatabase>

WaitingRoomWindow::WaitingRoomWindow(uint8_t maxPlayers, QWidget *parent)
    : BaseLobby(parent),
      localPlayerReady(false),
      currentPage(0),
      animationFrame(0),
      customFontId(-1),
      max_players(maxPlayers) {
    setWindowTitle("Need for Speed 2D - Sala de Espera");
    setFixedSize(700, 700);

    // Fuente
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");

    // Fondo (uso la ruta del branch principal)
    backgroundImage.load("assets/img/lobby/window_covers/waiting.jpeg");
    if (!backgroundImage.isNull()) {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    setupUI();

    // Timer para animación
    animationTimer = new QTimer(this);
    connect(animationTimer, &QTimer::timeout, this, &WaitingRoomWindow::updateReadyAnimation);
    animationTimer->start(500);

    // Inicializo slots vacíos con max_players (lobby_aux) manteniendo el local si luego se setea
    for (int i = 0; i < max_players; i++) {
        addPlayer("Esperando...", "", false);
    }

    updatePaginationButtons();
}

int WaitingRoomWindow::getCardsPerPage() const { return 4; }

void WaitingRoomWindow::setupUI() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    statusLabel = new QLabel("Esperando que todos esten listos...", this);
    if (customFontId != -1) {
        QFont statusFont = customFont; statusFont.setPointSize(12); statusLabel->setFont(statusFont);
    }
    statusLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 150); padding: 10px; border-radius: 5px;");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setGeometry(100, 100, 500, 40);

    playersPanel = new QWidget(this);
    playersPanel->setStyleSheet("background-color: rgba(0, 0, 0, 200); border: 2px solid white; border-radius: 5px;");
    playersPanel->setGeometry(50, 160, 600, 380);
    playersPanel->lower();

    createPlayerCards();

    prevPageButton = new QPushButton("◄", this);
    if (customFontId != -1) { QFont btnFont = customFont; btnFont.setPointSize(16); prevPageButton->setFont(btnFont); }
    prevPageButton->setStyleSheet(
        "QPushButton {background-color: rgba(0,0,0,230); color: white; border:2px solid white; border-radius:5px; padding:5px;}"
        "QPushButton:hover {background-color:#333333; border:2px solid #00FF00;}"
        "QPushButton:disabled {background-color:#555555; color:#888888;}"
    );
    prevPageButton->setCursor(Qt::PointingHandCursor);
    prevPageButton->setGeometry(50, 550, 40, 40);
    connect(prevPageButton, &QPushButton::clicked, this, &WaitingRoomWindow::onPreviousPage);

    nextPageButton = new QPushButton("►", this);
    if (customFontId != -1) { QFont btnFont = customFont; btnFont.setPointSize(16); nextPageButton->setFont(btnFont); }
    nextPageButton->setStyleSheet(
        "QPushButton {background-color: rgba(0,0,0,230); color: white; border:2px solid white; border-radius:5px; padding:5px;}"
        "QPushButton:hover {background-color:#333333; border:2px solid #00FF00;}"
        "QPushButton:disabled {background-color:#555555; color:#888888;}"
    );
    nextPageButton->setCursor(Qt::PointingHandCursor);
    nextPageButton->setGeometry(610, 550, 40, 40);
    connect(nextPageButton, &QPushButton::clicked, this, &WaitingRoomWindow::onNextPage);

    pageLabel = new QLabel("1 / 1", this);
    if (customFontId != -1) { QFont pageFont = customFont; pageFont.setPointSize(10); pageLabel->setFont(pageFont); }
    pageLabel->setStyleSheet("color: white; background-color: rgba(0,0,0,150); padding:5px; border-radius:3px;");
    pageLabel->setAlignment(Qt::AlignCenter);
    pageLabel->setGeometry(310, 555, 80, 30);

    readyButton = new QPushButton("Listo!", this);
    if (customFontId != -1) { QFont btnFont = customFont; btnFont.setPointSize(14); readyButton->setFont(btnFont); }
    readyButton->setStyleSheet(
        "QPushButton {background-color: rgba(0,0,0,230); color:white; border:2px solid white; border-radius:5px; padding:15px;}"
        "QPushButton:hover {background-color:#006600; border:2px solid #00FF00;}"
        "QPushButton:checked {background-color:#00AA00; border:3px solid #00FF00; color:#00FF00;}"
    );
    readyButton->setCursor(Qt::PointingHandCursor);
    readyButton->setCheckable(true);
    readyButton->setGeometry(470, 610, 180, 60);
    connect(readyButton, &QPushButton::clicked, this, &WaitingRoomWindow::onReadyClicked);

    startButton = new QPushButton("Iniciar!", this);
    if (customFontId != -1) { QFont btnFont = customFont; btnFont.setPointSize(14); startButton->setFont(btnFont); }
    startButton->setStyleSheet(
        "QPushButton {background-color: rgba(0,0,0,230); color:white; border:2px solid white; border-radius:5px; padding:15px;}"
        "QPushButton:hover {background-color:#006600; border:2px solid #FFD700;}"
        "QPushButton:disabled {background-color:#555555; color:#888888; border:2px solid #666666;}"
    );
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setEnabled(false);
    startButton->setGeometry(260, 610, 180, 60);
    connect(startButton, &QPushButton::clicked, this, &WaitingRoomWindow::onStartClicked);

    backButton = new QPushButton("Salir", this);
    if (customFontId != -1) { QFont btnFont = customFont; btnFont.setPointSize(14); backButton->setFont(btnFont); }
    backButton->setStyleSheet(
        "QPushButton {background-color: rgba(0,0,0,230); color:white; border:2px solid white; border-radius:5px; padding:15px;}"
        "QPushButton:hover {background-color:#333333; border:2px solid #FF0000;}"
    );
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setGeometry(50, 610, 180, 60);
    setupMusicControl();
    connect(backButton, &QPushButton::clicked, this, &WaitingRoomWindow::onBackClicked);
}

void WaitingRoomWindow::createPlayerCards() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0), 10);
        }
    }
    for (int i = 0; i < 4; i++) {
        PlayerCardWidgets card;
        int cardY = 180 + i * 90;
        card.container = new QWidget(this);
        card.container->setGeometry(70, cardY, 560, 80);
        card.statusCircle = new QWidget(card.container);
        card.statusCircle->setGeometry(12, 32, 16, 16);
        card.statusCircle->setStyleSheet("background-color: rgb(100, 100, 100); border-radius: 8px;");
        card.nameLabel = new QLabel("", card.container);
        card.nameLabel->setFont(customFont);
        card.nameLabel->setStyleSheet("color: white; background-color: transparent;");
        card.nameLabel->setGeometry(50, 20, 300, 20);
        card.carLabel = new QLabel("", card.container);
        card.carLabel->setFont(customFont);
        card.carLabel->setStyleSheet("color: rgb(255, 215, 0); background-color: transparent;");
        card.carLabel->setGeometry(50, 45, 300, 20);
        card.statusLabel = new QLabel("", card.container);
        card.statusLabel->setFont(customFont);
        card.statusLabel->setStyleSheet("color: rgb(255, 255, 0); background-color: transparent; font-weight: bold;");
        card.statusLabel->setGeometry(410, 35, 140, 20);
        card.statusLabel->setAlignment(Qt::AlignRight);
        playerCardWidgets.push_back(card);
    }
}

void WaitingRoomWindow::addPlayerByName(const QString& name) {
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i].name == "Esperando...") {
            players[i].name = name;
            players[i].carName = "Sin seleccionar";
            players[i].isReady = false;
            players[i].isLocal = false;
            player_name_to_index[name] = i;
            updatePlayerDisplay();
            updateStartButtonState();
            return;
        }
    }
}

void WaitingRoomWindow::removePlayerByName(const QString& name) {
    auto it = player_name_to_index.find(name);
    if (it == player_name_to_index.end()) return;
    int index = it->second;
    players[index].name = "Esperando...";
    players[index].carName = "";
    players[index].isReady = false;
    player_name_to_index.erase(it);
    updatePlayerDisplay();
    updateStartButtonState();
}

void WaitingRoomWindow::setPlayerReadyByName(const QString& name, bool ready) {
    auto it = player_name_to_index.find(name);
    if (it == player_name_to_index.end()) return;
    int index = it->second;
    if (index < 0 || index >= static_cast<int>(players.size())) return;
    players[index].isReady = ready;
    updatePlayerDisplay();
    updateStartButtonState();
}

void WaitingRoomWindow::setPlayerCarByName(const QString& name, const QString& car) {
    auto it = player_name_to_index.find(name);
    if (it == player_name_to_index.end()) return;
    int index = it->second;
    players[index].carName = car;
    updatePlayerDisplay();
}

void WaitingRoomWindow::updateStartButtonState() {
    int activePlayers = 0; bool allReady = true;
    for (const auto& p : players) {
        if (p.name != "Esperando...") {
            activePlayers++; if (!p.isReady) allReady = false; }
    }
    bool canStart = (activePlayers >= 2) && allReady;
    startButton->setEnabled(canStart);
    if (canStart) {
        statusLabel->setText("Todos listos! El host puede iniciar");
        statusLabel->setStyleSheet("color:#00FF00; background-color: rgba(0,0,0,150); padding:10px; border-radius:5px;");
    } else if (activePlayers < 2) {
        statusLabel->setText("Esperando mas jugadores...");
        statusLabel->setStyleSheet("color:#FFD700; background-color: rgba(0,0,0,150); padding:10px; border-radius:5px;");
    } else {
        statusLabel->setText("Esperando que todos esten listos...");
        statusLabel->setStyleSheet("color:#FFD700; background-color: rgba(0,0,0,150); padding:10px; border-radius:5px;");
    }
}

void WaitingRoomWindow::addPlayer(const QString& name, const QString& car, bool isLocal) {
    PlayerInfo player{ name, car, false, isLocal };
    players.push_back(player);
    updatePlayerDisplay();
}

void WaitingRoomWindow::setPlayerReady(int playerIndex, bool ready) {
    if (playerIndex >= 0 && static_cast<size_t>(playerIndex) < players.size()) {
        players[playerIndex].isReady = ready;
        updatePlayerDisplay();
        updateStartButtonState();
    }
}

void WaitingRoomWindow::setLocalPlayerInfo(const QString& name, const QString& car) {
    // Buscar primer slot "Esperando..." y asignar local
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i].isLocal) { // ya hay uno
            players[i].name = name + " (Tu)";
            players[i].carName = car;
            updatePlayerDisplay();
            return;
        }
    }
    for (size_t i = 0; i < players.size(); i++) {
        if (players[i].name == "Esperando...") {
            players[i].name = name + " (Tu)";
            players[i].carName = car;
            players[i].isLocal = true;
            player_name_to_index[name] = i; // map sin sufijo
            updatePlayerDisplay();
            return;
        }
    }
}

void WaitingRoomWindow::updatePlayerDisplay() {
    int startIdx = currentPage * getCardsPerPage();
    for (int i = 0; i < getCardsPerPage(); i++) {
        int playerIdx = startIdx + i;
        if (playerIdx < static_cast<int>(players.size())) {
            const PlayerInfo& player = players[playerIdx];
            PlayerCardWidgets& card = playerCardWidgets[i];
            card.container->show();
            QString bgColor = player.isLocal ? "rgba(0,50,100,200)" : "rgba(0,0,0,180)";
            QString borderColor = player.isReady ? "rgb(0,255,0)" : "rgb(255,255,255)";
            card.container->setStyleSheet(QString("background-color:%1; border:2px solid %2; border-radius:5px;").arg(bgColor, borderColor));
            if (player.isReady) {
                int pulseSize = 15 + (animationFrame % 2) * 3;
                card.statusCircle->setStyleSheet(QString("background-color: rgb(0,255,0); border-radius:%1px; width:%2px; height:%2px;").arg(pulseSize/2).arg(pulseSize));
                card.statusCircle->setGeometry(20 - pulseSize/2, 40 - pulseSize/2, pulseSize, pulseSize);
            } else {
                card.statusCircle->setStyleSheet("background-color: rgb(100,100,100); border-radius:8px;");
                card.statusCircle->setGeometry(12, 32, 16, 16);
            }
            card.nameLabel->setText(player.name);
            if (!player.carName.isEmpty()) {
                card.carLabel->setText("Auto: " + player.carName);
                card.carLabel->setStyleSheet("color: rgb(255,215,0); background-color: transparent;");
            } else {
                card.carLabel->setText("Esperando jugador...");
                card.carLabel->setStyleSheet("color: rgb(150,150,150); background-color: transparent;");
            }
            QString status = player.isReady ? "LISTO" : (player.name.contains("Esperando") ? "" : "No listo");
            card.statusLabel->setText(status);
            if (player.isReady) {
                card.statusLabel->setStyleSheet("color: rgb(0,255,0); background-color: transparent; font-weight: bold;");
            } else {
                card.statusLabel->setStyleSheet("color: rgb(255,255,0); background-color: transparent; font-weight: bold;");
            }
        } else {
            playerCardWidgets[i].container->hide();
        }
    }
    updatePaginationButtons();
}

void WaitingRoomWindow::updatePaginationButtons() {
    int totalPages = (players.size() + getCardsPerPage() - 1) / getCardsPerPage();
    if (totalPages == 0) totalPages = 1;
    prevPageButton->setEnabled(currentPage > 0);
    nextPageButton->setEnabled(currentPage < totalPages - 1);
    pageLabel->setText(QString("%1 / %2").arg(currentPage + 1).arg(totalPages));
}

void WaitingRoomWindow::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    if (!backgroundImage.isNull()) {
        painter.drawPixmap(0, 0, backgroundImage);
    } else {
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor(20,20,60));
        gradient.setColorAt(1, QColor(10,10,30));
        painter.fillRect(rect(), gradient);
    }
    QWidget::paintEvent(event);
}

void WaitingRoomWindow::updateReadyAnimation() {
    animationFrame++;
    updatePlayerDisplay();
}

void WaitingRoomWindow::onPreviousPage() {
    if (currentPage > 0) { currentPage--; updatePlayerDisplay(); }
}

void WaitingRoomWindow::onNextPage() {
    int totalPages = (players.size() + getCardsPerPage() - 1) / getCardsPerPage();
    if (currentPage < totalPages - 1) { currentPage++; updatePlayerDisplay(); }
}

void WaitingRoomWindow::onReadyClicked() {
    localPlayerReady = readyButton->isChecked();
    readyButton->setText(localPlayerReady ? "Cancelar" : "Listo!");
    emit readyToggled(localPlayerReady);
    std::cout << "Estado listo: " << (localPlayerReady ? "SI" : "NO") << std::endl;
}

void WaitingRoomWindow::onStartClicked() {
    std::cout << "Iniciando partida..." << std::endl;
    emit startGameRequested();
}

void WaitingRoomWindow::onBackClicked() {
    auto reply = QMessageBox::question(this,
        "Salir de la Partida",
        "¿Estás seguro de que quieres salir?\n\nSi eres el único jugador o el host, la partida se cancelará.",
        QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) return;
    emit backRequested();
}

WaitingRoomWindow::~WaitingRoomWindow() {
    if (animationTimer) {
        animationTimer->stop();
        delete animationTimer;
    }
}