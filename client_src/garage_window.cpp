#include "garage_window.h"
#include <iostream>
#include <QPainter>
#include <QFontDatabase>

GarageWindow::GarageWindow(QWidget *parent)
    : QWidget(parent), currentCarIndex(0), customFontId(-1) {
    
    setWindowTitle("Need for Speed 2D - Garage");
    setFixedSize(700, 700);
    
    // Cargar fuente
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");
    
    // Cargar fondo
    backgroundImage.load("assets/img/race.png");
    if (!backgroundImage.isNull()) {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    
    loadCars();
    setupUI();
    updateCarDisplay();
}

void GarageWindow::loadCars() {
    // autos
    cars = {
        {"Leyenda Urbana", "assets/img/autos/escarabajo.png", 40, 60, 80, 40},
        {"Brisa", "assets/img/autos/convertible.png", 60, 60, 60, 40},
        {"J-Classic 600", "assets/img/autos/carro-verde.png", 20, 60, 100, 40},
        {"Cavallo V8", "assets/img/autos/carro-rojo.png", 100, 80, 40, 40},
        {"Senator", "assets/img/autos/carro-azul.png", 60, 40, 20, 100},
        {"Nómada", "assets/img/autos/pickup.png", 40, 40, 40, 80},
        {"Stallion GT", "assets/img/autos/carro-rojo-2.png", 60, 80, 20, 60}
    };
}

void GarageWindow::setupUI() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }
    
    // Nombre del auto
    carNameLabel = new QLabel("", this);
    if (customFontId != -1) {
        QFont nameFont = customFont;
        nameFont.setPointSize(24);
        carNameLabel->setFont(nameFont);
    }

    carNameLabel->setStyleSheet("color: #f4f4f4; background-color: rgba(0, 0, 0, 230); padding: 10px; border-radius: 5px;");
    carNameLabel->setAlignment(Qt::AlignCenter);
    carNameLabel->setGeometry(188, 100, 300, 50);
    
    // Imagen del auto
    carImageLabel = new QLabel(this);
    carImageLabel->setStyleSheet("background-color: rgba(0, 0, 0, 155); border: 3px solid white; border-radius: 10px;");
    carImageLabel->setGeometry(210, 170, 250, 214);
    carImageLabel->setScaledContents(true);
    carImageLabel->setAlignment(Qt::AlignCenter);
    

    // panel de las stats
    statsPanel = new QWidget(this);
    statsPanel->setStyleSheet("background-color: rgba(0, 0, 0, 230); border: 2px solid white; border-radius: 5px;");
    statsPanel->setGeometry(50, 400, 600, 150);
    createStatLabels();
    
    // Botón anterior
    prevButton = new QPushButton("◄", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(20);
        prevButton->setFont(btnFont);
    }
    prevButton->setStyleSheet(
        "QPushButton {"
    "   background-color: rgba(0, 0, 0, 230);" 
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333;"
        "   border: 2px solid #00FF00;"
        "}"
    );
    prevButton->setCursor(Qt::PointingHandCursor);
    prevButton->setGeometry(50, 240, 80, 80);
    connect(prevButton, &QPushButton::clicked, this, &GarageWindow::onPreviousCar);
    
    // Botón siguiente
    nextButton = new QPushButton("►", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(20);
        nextButton->setFont(btnFont);
    }
    nextButton->setStyleSheet(
        "QPushButton {"
    "   background-color: rgba(0, 0, 0, 230);" 
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333;"
        "   border: 2px solid #00FF00;"
        "}"
    );
    nextButton->setCursor(Qt::PointingHandCursor);
    nextButton->setGeometry(570, 240, 80, 80);
    connect(nextButton, &QPushButton::clicked, this, &GarageWindow::onNextCar);
    
    // Botones inferiores
    selectButton = new QPushButton("Seleccionar", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        selectButton->setFont(btnFont);
    }
    selectButton->setStyleSheet(
        "QPushButton {"
    "   background-color: rgba(0, 0, 0, 230);" 
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 15px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #006600;"
        "   border: 2px solid #00FF00;"
        "}"
    );
    selectButton->setCursor(Qt::PointingHandCursor);
    selectButton->setGeometry(50, 590, 180, 60);
    connect(selectButton, &QPushButton::clicked, this, &GarageWindow::onSelectCar);
    
    backButton = new QPushButton("Volver", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        backButton->setFont(btnFont);
    }
    backButton->setStyleSheet(
        "QPushButton {"
    "   background-color: rgba(0, 0, 0, 230);" 
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 15px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333;"
        "   border: 2px solid #FF0000;"
        "}"
    );
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setGeometry(470, 590, 180, 60);
    connect(backButton, &QPushButton::clicked, this, &GarageWindow::onBackClicked);
}

void GarageWindow::createStatLabels() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0), 10);
        }
    }
    
    // Crear labels y barras para cada estadística
    const QStringList statNames = {"Velocidad", "Aceleracion", "Manejo", "Resistencia"};
    int statsY = 410;
    int spacing = 35;
    
    for (int i = 0; i < 4; i++) {
        // Label del nombre
        QLabel* nameLabel = new QLabel(statNames[i], this);
        nameLabel->setFont(customFont);
        nameLabel->setStyleSheet("color: white; background-color: transparent;");
        nameLabel->setGeometry(80, statsY + i * spacing, 100, 20);
        statNameLabels.push_back(nameLabel);
        
        // Widget para la barra de progreso
        QWidget* barBg = new QWidget(this);
        barBg->setStyleSheet("background-color: rgb(50, 50, 50); border: 2px solid white;");
        barBg->setGeometry(180, statsY + i * spacing + 5, 400, 20);
        statBarBackgrounds.push_back(barBg);
        
        // Widget para el relleno de la barra
        QWidget* barFill = new QWidget(barBg);
        barFill->setGeometry(0, 0, 0, 20);
        statBarFills.push_back(barFill);
        
        // Label del valor
        QLabel* valueLabel = new QLabel("0", this);
        valueLabel->setFont(customFont);
        valueLabel->setStyleSheet("color: white; background-color: transparent;");
        valueLabel->setGeometry(590, statsY + i * spacing + 5, 50, 20);
        statValueLabels.push_back(valueLabel);
    }
}

void GarageWindow::updateCarDisplay() {
    if (currentCarIndex >= cars.size()) return;
    
    const CarInfo& car = cars[currentCarIndex];
    
    carNameLabel->setText(car.name);
    
    // Cargar imagen del auto
    QPixmap carImage(car.imagePath);
    if (carImage.isNull()) {
        // Placeholder si no existe la imagen
        carImage = QPixmap(350, 200);
        carImage.fill(QColor(50, 50, 50));
        QPainter p(&carImage);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 16));
        p.drawText(carImage.rect(), Qt::AlignCenter, "Auto\n" + car.name);
    }
    carImageLabel->setPixmap(carImage);
    
    // Actualizar estadísticas en los widgets
    const int stats[4] = {car.speed, car.acceleration, car.handling, car.durability};
    
    for (int i = 0; i < 4; i++) {
        int value = stats[i];
        int fillWidth = (400 * value) / 100;
        
        // Actualizar ancho de la barra
        statBarFills[i]->setGeometry(0, 0, fillWidth, 20);
        
        // Actualizar color según valor
        QColor barColor;
        if (value >= 80) barColor = QColor(0, 255, 0);
        else if (value >= 60) barColor = QColor(255, 255, 0);
        else barColor = QColor(255, 100, 0);
        
        statBarFills[i]->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(barColor.red()).arg(barColor.green()).arg(barColor.blue()));
        
        // Actualizar valor numérico
        statValueLabels[i]->setText(QString::number(value));
    }
}

void GarageWindow::paintEvent(QPaintEvent* event) {
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

void GarageWindow::onPreviousCar() {
    if (currentCarIndex == 0) {
        currentCarIndex = cars.size() - 1;
    } else {
        currentCarIndex--;
    }
    updateCarDisplay();
}

void GarageWindow::onNextCar() {
    currentCarIndex++;
    if (currentCarIndex >= cars.size()) {
        currentCarIndex = 0;
    }
    updateCarDisplay();
}

void GarageWindow::onSelectCar() {
    std::cout << "Auto seleccionado: " << cars[currentCarIndex].name.toStdString() << std::endl;
    emit carSelected(static_cast<int>(currentCarIndex));
}

void GarageWindow::onBackClicked() {
    emit backRequested();
}

GarageWindow::~GarageWindow() {
}