#include "lobby_window.h"
#include "match_selection_window.h"
#include "garage_window.h"
#include "waiting_room_window.h"
#include <iostream>
#include <QPainter>
#include <QCloseEvent>
#include <QFontDatabase>
#include <SDL2/SDL.h>

LobbyWindow::LobbyWindow(QWidget *parent) 
    : QWidget(parent), 
      backgroundMusic(nullptr),
      audioInitialized(false),
      customFontId(-1),
      matchSelectionWindow(nullptr),
      garageWindow(nullptr),
      waitingRoomWindow(nullptr) {
    
    // 1. Configuración de la Ventana Qt
    setWindowTitle("Need for Speed 2D - Lobby");
    setFixedSize(700, 700);
    
    // 2. Cargar fuente personalizada
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");
    if (customFontId == -1) {
        std::cerr << "Error: No se pudo cargar la fuente Arcade Classic" << std::endl;
    } else {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            std::cout << "Fuente cargada: " << fontFamilies.at(0).toStdString() << std::endl;
        }
    }
    
    // 3. Cargar imagen de fondo con Qt
    backgroundImage.load("assets/img/menu.png");
    if (backgroundImage.isNull()) {
        std::cerr << "Error: No se pudo cargar assets/img/menu.png" << std::endl;
    } else {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        std::cout << "Imagen de fondo cargada correctamente" << std::endl;
    }
    
    // 4. Inicializar audio con SDL
    initAudio();
    loadMusic("assets/music/Tokyo-Drift.mp3");
    
    // 5. Crear la fuente para usar en los widgets
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }
    
    // 6. Botones
    connectButton = new QPushButton("Jugar", this);
    
    if (customFontId != -1) {
        QFont buttonFont = customFont;
        buttonFont.setPointSize(14);
        connectButton->setFont(buttonFont);
    }
    
    connectButton->setStyleSheet(
        "QPushButton {"
        "   font-size: 14pt; "
        "   padding: 15px 30px; "
        "   background-color: #000000; "
        "   color: white; "
        "   border: 2px solid white; "
        "   border-radius: 5px; "
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333; "
        "   border: 2px solid #00FF00; "
        "}"
        "QPushButton:disabled {"
        "   background-color: #555555; "
        "   color: #888888; "
        "   border: 2px solid #666666; "
        "}"
    );
    connectButton->setCursor(Qt::PointingHandCursor);
    
    quitButton = new QPushButton("Salir", this);
    
    if (customFontId != -1) {
        QFont quitFont = customFont;
        quitFont.setPointSize(14);
        quitButton->setFont(quitFont);
    }
    
    quitButton->setStyleSheet(
        "QPushButton {"
        "   font-size: 14pt; "
        "   padding: 15px 30px; "
        "   background-color: #000000; "
        "   color: white; "
        "   border: 2px solid white; "
        "   border-radius: 5px; "
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333; "
        "   border: 2px solid #FF0000; "
        "}"
    );
    quitButton->setCursor(Qt::PointingHandCursor);
    
    // 7. Layout sin layout automático
    setLayout(nullptr);
    
    connectButton->setGeometry(50, 600, 270, 60);
    quitButton->setGeometry(375, 600, 270, 60);
    
    // 8. Conexión de señales
    connect(connectButton, &QPushButton::clicked, this, &LobbyWindow::onConnectClicked);
    connect(quitButton, &QPushButton::clicked, this, &LobbyWindow::close);
}

void LobbyWindow::initAudio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Error inicializando SDL Audio: " << SDL_GetError() << std::endl;
        return;
    }
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Error inicializando SDL_mixer: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return;
    }
    
    audioInitialized = true;
    std::cout << "SDL Audio inicializado correctamente" << std::endl;
}

void LobbyWindow::loadMusic(const char* musicPath) {
    if (!audioInitialized) return;
    
    backgroundMusic = Mix_LoadMUS(musicPath);
    if (!backgroundMusic) {
        std::cerr << "Error cargando música: " << Mix_GetError() << std::endl;
        return;
    }
    
    if (Mix_PlayMusic(backgroundMusic, -1) == -1) {
        std::cerr << "Error reproduciendo música: " << Mix_GetError() << std::endl;
        return;
    }
    
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2);
    std::cout << "Música de fondo iniciada en loop" << std::endl;
}

void LobbyWindow::paintEvent(QPaintEvent* event) {
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

void LobbyWindow::closeEvent(QCloseEvent* event) {
    cleanupAudio();
    QWidget::closeEvent(event);
}

void LobbyWindow::cleanupAudio() {
    if (backgroundMusic) {
        Mix_HaltMusic();
        Mix_FreeMusic(backgroundMusic);
        backgroundMusic = nullptr;
    }
    
    if (audioInitialized) {
        Mix_CloseAudio();
        SDL_Quit();
        audioInitialized = false;
    }
}

LobbyWindow::~LobbyWindow() {
    cleanupAudio();
    
    // Limpiar ventanas secundarias
    if (matchSelectionWindow) delete matchSelectionWindow;
    if (garageWindow) delete garageWindow;
    if (waitingRoomWindow) delete waitingRoomWindow;
}

void LobbyWindow::onConnectClicked() {
    connectButton->setEnabled(false);
    
    // Fade out de música
    if (backgroundMusic) {
        Mix_FadeOutMusic(1000);
    }
    
    std::cout << "Abriendo selección de partidas..." << std::endl;
    
    // Crear ventana de selección de partidas
    matchSelectionWindow = new MatchSelectionWindow();
    
    // Conectar señales
    connect(matchSelectionWindow, &MatchSelectionWindow::joinMatchRequested,
            this, &LobbyWindow::onJoinMatch);
    connect(matchSelectionWindow, &MatchSelectionWindow::createMatchRequested,
            this, &LobbyWindow::onCreateMatch);
    connect(matchSelectionWindow, &MatchSelectionWindow::backToLobby,
            this, &LobbyWindow::onBackFromMatchSelection);
    
    // Ocultar lobby y mostrar selección
    this->hide();
    matchSelectionWindow->show();
}

void LobbyWindow::onJoinMatch(const QString& matchId) {
    std::cout << "Uniéndose a partida: " << matchId.toStdString() << std::endl;
    
    // Cerrar selección de partidas
    if (matchSelectionWindow) {
        matchSelectionWindow->hide();
    }
    
    // Abrir garage para seleccionar auto
    openGarage();
}

void LobbyWindow::onCreateMatch() {
    std::cout << "Creando nueva partida..." << std::endl;
    
    // TODO: Aquí pedirías el nombre de la partida al usuario
    // Por ahora, directamente abrimos el garage
    
    if (matchSelectionWindow) {
        matchSelectionWindow->hide();
    }
    
    openGarage();
}

void LobbyWindow::openGarage() {
    garageWindow = new GarageWindow();
    
    connect(garageWindow, &GarageWindow::carSelected,
            this, &LobbyWindow::onCarSelected);
    connect(garageWindow, &GarageWindow::backRequested,
            this, &LobbyWindow::onBackFromGarage);
    
    garageWindow->show();
}

void LobbyWindow::onCarSelected(int carIndex) {
    std::cout << "Auto seleccionado (índice " << carIndex << "), abriendo sala de espera..." << std::endl;
    
    if (garageWindow) {
        garageWindow->hide();
    }
    
    // Abrir sala de espera
    waitingRoomWindow = new WaitingRoomWindow();
    
    connect(waitingRoomWindow, &WaitingRoomWindow::startGameRequested,
            this, &LobbyWindow::onStartGame);
    connect(waitingRoomWindow, &WaitingRoomWindow::backRequested,
            this, &LobbyWindow::onBackFromWaitingRoom);
    
    waitingRoomWindow->show();
}

void LobbyWindow::onStartGame() {
    std::cout << "¡INICIANDO JUEGO!" << std::endl;
    
    // Aquí iniciarías el juego real
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
    }
    
    // TODO: Lanzar ventana de juego
}

void LobbyWindow::onBackFromMatchSelection() {
    if (matchSelectionWindow) {
        matchSelectionWindow->close();
        delete matchSelectionWindow;
        matchSelectionWindow = nullptr;
    }
    
    connectButton->setEnabled(true);
    this->show();
    
    // Reiniciar música
    if (backgroundMusic && audioInitialized) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
}

void LobbyWindow::onBackFromGarage() {
    if (garageWindow) {
        garageWindow->close();
        delete garageWindow;
        garageWindow = nullptr;
    }
    
    // Volver a selección de partidas
    if (matchSelectionWindow) {
        matchSelectionWindow->show();
    }
}

void LobbyWindow::onBackFromWaitingRoom() {
    if (waitingRoomWindow) {
        waitingRoomWindow->close();
        delete waitingRoomWindow;
        waitingRoomWindow = nullptr;
    }
    
    // Volver a garage
    if (garageWindow) {
        garageWindow->show();
    }
}