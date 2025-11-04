#include "lobby_window.h"
#include <iostream>
#include <QPainter>
#include <QCloseEvent>
#include <QFontDatabase>
#include <SDL2/SDL.h>

LobbyWindow::LobbyWindow(QWidget *parent) 
    : QWidget(parent), 
      backgroundMusic(nullptr),
      audioInitialized(false),
      customFontId(-1) {
    
    // Configuración de la Ventana Qt
    setWindowTitle("Need for Speed 2D - Lobby");
    setFixedSize(700, 700);
    
    // fuente personalizada
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");
    if (customFontId == -1) {
        std::cerr << "Error: No se pudo cargar la fuente Arcade Classic" << std::endl;
        std::cerr << "Asegúrate de que el archivo esté en: assets/fonts/ARCADE_N.TTF" << std::endl;
    } else {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            std::cout << "Fuente cargada: " << fontFamilies.at(0).toStdString() << std::endl;
        }
    }
    
    // Cargar imagen de fondo con Qt
    backgroundImage.load("assets/img/menu.png");
    if (backgroundImage.isNull()) {
        std::cerr << "Error: No se pudo cargar assets/img/lobby.jpeg" << std::endl;
        std::cerr << "Verifica que la ruta sea correcta desde donde ejecutas el programa" << std::endl;
    } else {
        
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        std::cout << "Imagen de fondo cargada correctamente" << std::endl;
    }
    
    // reproductor de audio con SDL porque con QT no me dejó
    initAudio();
    loadMusic("assets/music/Tokyo-Drift.mp3");
    
    
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }
    
 
    if (customFontId != -1) {
        QFont titleFont = customFont;
        titleFont.setPointSize(60);
        titleFont.setBold(true);  
        
    }
    

    
    connectButton = new QPushButton("Jugar", this);
    
    //fuente spbre el boton
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
    
    //fuente spbre botón salir
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
    
   
    // Usamos un layout principal sin layout, posicionamiento absoluto
    setLayout(nullptr);
    
    
    // Botón conectar en esquina inferior izquierda
    connectButton->setGeometry(50, 600, 270, 60);
    
    // Botón salir en esquina inferior derecha
    quitButton->setGeometry(375, 600, 270, 60);
    
  
    connect(connectButton, &QPushButton::clicked, this, &LobbyWindow::onConnectClicked);
    connect(quitButton, &QPushButton::clicked, this, &LobbyWindow::close);
}

void LobbyWindow::initAudio() {
    // Solo inicializar el subsistema de audio de SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Error inicializando SDL Audio: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Inicializar SDL_mixer
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
        std::cerr << "Intentando cargar desde: " << musicPath << std::endl;
        std::cerr << "Verifica que la ruta sea correcta desde donde ejecutas el programa" << std::endl;
        return;
    }
    
    // Reproducir en loop infinito
    if (Mix_PlayMusic(backgroundMusic, -1) == -1) {
        std::cerr << "Error reproduciendo música: " << Mix_GetError() << std::endl;
        return;
    }
    
    Mix_VolumeMusic(MIX_MAX_VOLUME / 2); // 50% de volumen
    std::cout << "Música de fondo iniciada en loop" << std::endl;
}

void LobbyWindow::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    
    if (!backgroundImage.isNull()) {
        //imagen de fondo
        painter.drawPixmap(0, 0, backgroundImage);
    } else {
        // fondo degradado si no carga la imagen
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
}

void LobbyWindow::onConnectClicked() {
    //statusLabel->setText("Intentando conectar...");
    connectButton->setEnabled(false);
    
    
    if (backgroundMusic) {
        Mix_FadeOutMusic(1000); // Fade out de 1 segundo
    }
    
    std::cout << "DEBUG: Conectar presionado. Iniciando cliente de red..." << std::endl;
    
    // AQUÍ integrarías la lógica de red
}