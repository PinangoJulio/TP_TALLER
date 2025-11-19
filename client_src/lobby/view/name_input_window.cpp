#include "name_input_window.h"
#include <iostream>
#include <QPainter>
#include <QFontDatabase>
#include <QRegularExpression>
// 1. QUITAR: #include <SDL2/SDL_mixer.h>

NameInputWindow::NameInputWindow(QWidget *parent)
    // 2. Llamar al constructor de  BaseLobby
    :  BaseLobby(parent), playerName("") {
    
    setWindowTitle("Need for Speed 2D - Ingrese su Nombre");
    setFixedSize(700, 700);
    
    // 3. Carga la fuente en el miembro heredado 'customFontId'
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");
    if (customFontId == -1) {
        std::cerr << "Error: No se pudo cargar la fuente Arcade Classic" << std::endl;
    }
    
    backgroundImage.load("assets/img/lobby/window_covers/nombre.png");
    if (!backgroundImage.isNull()) {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    
    setupUI();
}

NameInputWindow::~NameInputWindow() {
}

void NameInputWindow::setupUI() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }
    
    
    setupMusicControl(); 

    // Título
    titleLabel = new QLabel("Bienvenido", this);
    if (customFontId != -1) {
        QFont titleFont = customFont;
        titleFont.setPointSize(36);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);
    }
    titleLabel->setStyleSheet(
        "color: white; "
        "background-color: rgba(0, 0, 0, 230); "
        "padding: 20px; "
        "border-radius: 10px;"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setGeometry(100, 150, 500, 80);
    
    // Instrucciones
    instructionLabel = new QLabel("Ingrese su nombre de piloto:", this);
    if (customFontId != -1) {
        QFont instrFont = customFont;
        instrFont.setPointSize(14);
        instructionLabel->setFont(instrFont);
    }
    instructionLabel->setStyleSheet(
        "color: #FFD700; "
        "background-color: rgba(0, 0, 0, 180); "
        "padding: 10px; "
        "border-radius: 5px;"
    );
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setGeometry(150, 280, 400, 40);
    
    // Campo de entrada de nombre
    nameInput = new QLineEdit(this);
    if (customFontId != -1) {
        QFont inputFont = customFont;
        inputFont.setPointSize(16);
        nameInput->setFont(inputFont);
    }
    nameInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: rgba(255, 255, 255, 230); "
        "   color: black; "
        "   border: 3px solid white; "
        "   border-radius: 5px; "
        "   padding: 15px; "
        "   font-size: 16pt;"
        "}"
        "QLineEdit:focus {"
        "   border: 3px solid #ffffff; "
        "   background-color: rgba(255, 255, 255, 255);"
        "}"
    );
    nameInput->setPlaceholderText("Tu nombre aqui...");
    nameInput->setMaxLength(20);
    nameInput->setGeometry(150, 340, 400, 60);
    nameInput->setAlignment(Qt::AlignCenter);
    
    connect(nameInput, &QLineEdit::textChanged, this, &NameInputWindow::onTextChanged);
    connect(nameInput, &QLineEdit::returnPressed, this, &NameInputWindow::onConfirmClicked);
    
    // Label de error (inicialmente oculto)
    errorLabel = new QLabel("", this);
    if (customFontId != -1) {
        QFont errorFont = customFont;
        errorFont.setPointSize(10);
        errorLabel->setFont(errorFont);
    }
    errorLabel->setStyleSheet(
        "color: #FF4444; "
        "background-color: rgba(0, 0, 0, 200); "
        "padding: 8px; "
        "border-radius: 5px; "
        "border: 2px solid #FF4444;"
    );
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setGeometry(150, 410, 400, 35);
    errorLabel->hide();
    
    // Botón confirmar
    confirmButton = new QPushButton("Continuar", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(16);
        confirmButton->setFont(btnFont);
    }
    confirmButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230); "
        "   color: white; "
        "   border: 3px solid white; "
        "   border-radius: 5px; "
        "   padding: 20px; "
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background-color: #006600; "
        "   border: 3px solid #00FF00; "
        "}"
        "QPushButton:disabled {"
        "   background-color: #555555; "
        "   color: #888888; "
        "   border: 3px solid #666666; "
        "}"
    );
    confirmButton->setCursor(Qt::PointingHandCursor);
    confirmButton->setEnabled(false);
    confirmButton->setGeometry(250, 600, 200, 70);
    
    connect(confirmButton, &QPushButton::clicked, this, &NameInputWindow::onConfirmClicked);
    
   
    nameInput->setFocus();
}

bool NameInputWindow::validateName(const QString& name) {
    // Validar que no esté vacío
    if (name.trimmed().isEmpty()) {
        return false;
    }
    
    // Validar longitud mínima
    if (name.trimmed().length() < 3) {
        errorLabel->setText("El nombre debe tener al menos 3 caracteres");
        errorLabel->show();
        return false;
    }
    
    // Validar que solo contenga letras, números, espacios y guiones
    QRegularExpression regex("^[a-zA-Z0-9 _-]+$");
    if (!regex.match(name).hasMatch()) {
        errorLabel->setText("Solo letras, numeros, espacios, _ y -");
        errorLabel->show();
        return false;
    }
    
    errorLabel->hide();
    return true;
}

void NameInputWindow::onTextChanged(const QString& text) {
    bool isValid = validateName(text);
    confirmButton->setEnabled(isValid);
    
    
    if (isValid) {
        errorLabel->hide();
    }
}

void NameInputWindow::onConfirmClicked() {
    QString name = nameInput->text().trimmed();
    
    if (validateName(name)) {
        playerName = name;
        std::cout << "Nombre de jugador confirmado: " << playerName.toStdString() << std::endl;
        emit nameConfirmed(playerName);
    }
}

QString NameInputWindow::getPlayerName() const {
    return playerName;
}

void NameInputWindow::paintEvent(QPaintEvent* event) {
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