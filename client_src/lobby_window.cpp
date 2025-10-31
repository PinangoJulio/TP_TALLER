#include "lobby_window.h"
#include <iostream>

LobbyWindow::LobbyWindow(QWidget *parent) : QWidget(parent) {
    // 1. Configuración de la Ventana
    setWindowTitle("Need for Speed 2D - Lobby");
    setMinimumSize(400, 300);

    // 2. Elementos de la Interfaz

    // Título del Juego (QLabel)
    titleLabel = new QLabel("🚗 NEED FOR SPEED TP 🚀", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 24pt; font-weight: bold; color: #FF0000;");

    // Etiqueta de Estado
    statusLabel = new QLabel("Esperando conexión...", this);
    statusLabel->setAlignment(Qt::AlignCenter);

    // Botón de Conexión
    connectButton = new QPushButton("Conectar al Servidor", this);

    // Botón de Salida
    quitButton = new QPushButton("Salir", this);

    // 3. Layout (Distribución de Elementos)
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(statusLabel);
    layout->addStretch(); // Espacio flexible
    layout->addWidget(connectButton);
    layout->addWidget(quitButton);

    // 4. Conexión de Señales y Slots (Eventos)
    // Cuando se hace click en connectButton, se llama a onConnectClicked()
    connect(connectButton, &QPushButton::clicked, this, &LobbyWindow::onConnectClicked);

    // Conexión del botón Salir para cerrar la ventana (función de Qt)
    connect(quitButton, &QPushButton::clicked, this, &LobbyWindow::close);
}

LobbyWindow::~LobbyWindow() {}

void LobbyWindow::onConnectClicked() {
    // Lógica temporal para mostrar que el botón funciona
    statusLabel->setText("Intentando conectar...");
    connectButton->setEnabled(false);

    // AQUÍ es donde integrarías la lógica de red:
    // 1. Intentar conectar al Servidor.
    // 2. Si es exitoso, ocultar el lobby y pasar a la pantalla de selección de auto.
    std::cout << "DEBUG: Conectar presionado. Iniciando cliente de red..." << std::endl;
}