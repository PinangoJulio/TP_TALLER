#include "match_selection_window.h"

#include <QCloseEvent>
#include <QFontDatabase>
#include <QPainter>
#include <QScrollBar>
#include <iostream>
#include <vector>

MatchSelectionWindow::MatchSelectionWindow(QWidget* parent)
    : BaseLobby(parent), selectedMatchId("") {
    setWindowTitle("Need for Speed 2D - Selección de Partida");
    setFixedSize(700, 700);

    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");

    backgroundImage.load("assets/img/lobby/window_covers/race.png");
    if (!backgroundImage.isNull()) {
        backgroundImage =
            backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    setupUI();
}

void MatchSelectionWindow::setupUI() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    // Título
    titleLabel = new QLabel("Partidas Disponibles", this);
    if (customFontId != -1) {
        QFont titleFont = customFont;
        titleFont.setPointSize(32);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
    }
    titleLabel->setStyleSheet(
        "color: white; background-color: rgba(0, 0, 0, 230); padding: 10px; border-radius: 5px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setGeometry(100, 30, 500, 60);

    // Lista de partidas
    matchList = new QListWidget(this);
    if (customFontId != -1) {
        QFont listFont = customFont;
        listFont.setPointSize(10);
        matchList->setFont(listFont);
    }

    matchList->setStyleSheet("QListWidget {"
                             "   background-color: rgba(0, 0, 0, 230);"
                             "   color: white;"
                             "   border: 2px solid white;"
                             "   border-radius: 5px;"
                             "   padding: 10px;"
                             "}"
                             "QListWidget::item {"
                             "   padding: 15px;"
                             "   border-bottom: 1px solid #444444;"
                             "   margin: 5px;"
                             "   border-radius: 3px;"
                             "}"
                             "QListWidget::item:hover {"
                             "   background-color: rgba(51, 51, 51, 200);"
                             "   border: 1px solid #00FF00;"
                             "}"
                             "QListWidget::item:selected {"
                             "   background-color: rgba(0, 255, 0, 100);"
                             "   border: 2px solid #00FF00;"
                             "}"
                             "QScrollBar:vertical {"
                             "   background: rgba(0, 0, 0, 150);"
                             "   width: 15px;"
                             "   border-radius: 5px;"
                             "}"
                             "QScrollBar::handle:vertical {"
                             "   background: #00FF00;"
                             "   border-radius: 5px;"
                             "}");
    matchList->setGeometry(50, 110, 600, 400);

    connect(matchList, &QListWidget::itemClicked, this, &MatchSelectionWindow::onMatchSelected);

    // Botón Actualizar
    refreshButton = new QPushButton("Actualizar", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        refreshButton->setFont(btnFont);
    }
    refreshButton->setStyleSheet("QPushButton {"
                                 "   background-color: rgba(0, 0, 0, 230);"
                                 "   color: white;"
                                 "   border: 2px solid white;"
                                 "   border-radius: 5px;"
                                 "   padding: 10px;"
                                 "}"
                                 "QPushButton:hover {"
                                 "   background-color: #333333;"
                                 "   border: 2px solid #0099FF;"
                                 "}");
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setGeometry(260, 530, 180, 60);

    connect(refreshButton, &QPushButton::clicked, this, [this]() { emit refreshRequested(); });

    joinButton = new QPushButton("Unirse", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        joinButton->setFont(btnFont);
    }
    joinButton->setStyleSheet("QPushButton {"
                              "   background-color: rgba(0, 0, 0, 230);"
                              "   color: white;"
                              "   border: 2px solid white;"
                              "   border-radius: 5px;"
                              "   padding: 15px;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #333333;"
                              "   border: 2px solid #00FF00;"
                              "}"
                              "QPushButton:disabled {"
                              "   background-color: #555555;"
                              "   color: #888888;"
                              "   border: 2px solid #666666;"
                              "}");
    joinButton->setCursor(Qt::PointingHandCursor);
    joinButton->setEnabled(false);
    joinButton->setGeometry(470, 600, 180, 60);
    connect(joinButton, &QPushButton::clicked, this, &MatchSelectionWindow::onJoinMatchClicked);

    createButton = new QPushButton("Crear Nueva", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        createButton->setFont(btnFont);
    }
    createButton->setStyleSheet("QPushButton {"
                                "   background-color: rgba(0, 0, 0, 230);"
                                "   color: white;"
                                "   border: 2px solid white;"
                                "   border-radius: 5px;"
                                "   padding: 15px;"
                                "}"
                                "QPushButton:hover {"
                                "   background-color: #333333;"
                                "   border: 2px solid #FFD700;"
                                "}");
    createButton->setCursor(Qt::PointingHandCursor);
    createButton->setGeometry(260, 600, 180, 60);
    connect(createButton, &QPushButton::clicked, this, &MatchSelectionWindow::onCreateMatchClicked);

    backButton = new QPushButton("Volver", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        backButton->setFont(btnFont);
    }
    backButton->setStyleSheet("QPushButton {"
                              "   background-color: rgba(0, 0, 0, 230);"
                              "   color: white;"
                              "   border: 2px solid white;"
                              "   border-radius: 5px;"
                              "   padding: 15px;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #333333;"
                              "   border: 2px solid #FF0000;"
                              "}");
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setGeometry(50, 600, 180, 60);
    connect(backButton, &QPushButton::clicked, this, &MatchSelectionWindow::onBackClicked);
    setupMusicControl();
}

void MatchSelectionWindow::updateGamesList(const std::vector<GameInfo>& games) {
    matchList->clear();
    selectedMatchId = "";
    joinButton->setEnabled(false);

    std::cout << "[MatchSelectionWindow] Actualizando lista con " << games.size() << " partidas"
              << std::endl;

    if (games.empty()) {
        QListWidgetItem* item = new QListWidgetItem("No hay partidas disponibles");
        item->setFlags(Qt::NoItemFlags);  // No seleccionable
        item->setForeground(QColor(150, 150, 150));
        matchList->addItem(item);
        return;
    }

    for (const auto& game : games) {
        QString gameName = QString::fromUtf8(game.game_name).trimmed();

        QString itemText = QString("🏁 %1 | %2/%3 jugadores")
                               .arg(gameName)
                               .arg(game.current_players)
                               .arg(game.max_players);

        if (game.is_started) {
            itemText += " | EN CURSO";
        }

        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, game.game_id);

        // Deshabilitar partidas llenas o ya iniciadas
        if (game.is_started || game.current_players >= game.max_players) {
            item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
            item->setForeground(QColor(150, 150, 150));
        }

        matchList->addItem(item);
    }
}

void MatchSelectionWindow::onMatchSelected(QListWidgetItem* item) {
    if (item && !item->text().contains("EN CURSO")) {
        uint16_t gameId = item->data(Qt::UserRole).toUInt();
        selectedMatchId = QString::number(gameId);
        joinButton->setEnabled(true);

        std::cout << "[MatchSelectionWindow] Partida seleccionada ID: " << gameId << std::endl;
    }
}

void MatchSelectionWindow::onJoinMatchClicked() {
    if (!selectedMatchId.isEmpty()) {
        std::cout << "[MatchSelectionWindow] Emitiendo señal de unirse a: "
                  << selectedMatchId.toStdString() << std::endl;
        emit joinMatchRequested(selectedMatchId);
    }
}

void MatchSelectionWindow::onCreateMatchClicked() {
    std::cout << "Creando nueva partida..." << std::endl;
    emit createMatchRequested();
}

void MatchSelectionWindow::onBackClicked() {
    std::cout << "Volviendo al lobby..." << std::endl;
    emit backToLobby();
}

void MatchSelectionWindow::paintEvent(QPaintEvent* event) {
    QPainter painter(this);

    if (!backgroundImage.isNull()) {
        painter.drawPixmap(0, 0, backgroundImage);
    } else {
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor(20, 20, 60));
        gradient.setColorAt(1, QColor(10, 10, 30));
        painter.fillRect(rect(), gradient);
    }

    QWidget::paintEvent(event);
}

void MatchSelectionWindow::closeEvent(QCloseEvent* event) {
    QWidget::closeEvent(event);
}

QListWidgetItem* MatchSelectionWindow::getSelectedItem() const {
    return matchList->currentItem();
}

MatchSelectionWindow::~MatchSelectionWindow() {}
