#include "garage_window.h"

#include <QFontDatabase>
#include <QPainter>
#include <iostream>
#include <string>

#include "common_src/config.h"

GarageWindow::GarageWindow(QWidget* parent) : BaseLobby(parent), currentCarIndex(0) {
    setWindowTitle("Need for Speed 2D - Garage");
    setFixedSize(700, 700);

    // Cargar fuente
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");

    // Cargar fondo
    backgroundImage.load("assets/img/lobby/window_covers/race.png");
    if (!backgroundImage.isNull()) {
        backgroundImage =
            backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    loadCars();
    setupUI();
    updateCarDisplay();
}

void GarageWindow::loadCars() {
    // Cargar configuración
    const char* path_config = "config.yaml";
    Configuration::load_path(path_config);

   
    cars.clear();

    try {
       
        auto carsNode = Configuration::get_node("cars");

        if (!carsNode.IsDefined() || !carsNode.IsSequence()) {
            std::cerr
                << "[GarageWindow] Error: 'cars' no encontrado o no es una secuencia en config.yaml"
                << std::endl;
            // Fallback: cargar datos hardcodeados
            loadDefaultCars();
            return;
        }

       
        for (size_t i = 0; i < carsNode.size(); i++) {
            YAML::Node carNode = carsNode[i];

            CarInfo car;
            car.name = QString::fromStdString(carNode["name"].template as<std::string>());
            car.imagePath = QString::fromStdString(carNode["image_path"].template as<std::string>());
            car.type = QString::fromStdString(carNode["type"].template as<std::string>());
            car.speed = carNode["speed"].template as<int>();
            car.acceleration = carNode["acceleration"].template as<int>();
            car.handling = carNode["handling"].template as<int>();
            car.durability = carNode["durability"].template as<int>();

            cars.push_back(car);

            std::cout << "[GarageWindow] Auto cargado: " << car.name.toStdString()
                      << " (tipo: " << car.type.toStdString() << ")" << std::endl;
        }

        std::cout << "[GarageWindow]   " << cars.size() << " autos cargados desde config.yaml"
                  << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[GarageWindow] Error cargando autos desde YAML: " << e.what() << std::endl;
        // Fallback: cargar datos hardcodeados
        loadDefaultCars();
    }
}

void GarageWindow::loadDefaultCars() {
    // ⚠️ Fallback: datos hardcodeados si falla la carga del YAML
    std::cout << "[GarageWindow] ⚠️ Usando autos por defecto (hardcoded)" << std::endl;

    cars = {{"Leyenda Urbana", "assets/img/lobby/autos/escarabajo.png", "classic", 70, 60, 65, 80},
            {"Brisa", "assets/img/lobby/autos/convertible.png", "sport", 90, 85, 70, 60},
            {"J-Classic 600", "assets/img/lobby/autos/carro-verde.png", "classic", 70, 60, 65, 80},
            {"Cavallo V8", "assets/img/lobby/autos/carro-rojo.png", "sport", 90, 85, 70, 60},
            {"Senator", "assets/img/lobby/autos/carro-azul.png", "classic", 70, 60, 65, 80},
            {"Nómada", "assets/img/lobby/autos/pickup.png", "truck", 60, 50, 55, 90},
            {"Stallion GT", "assets/img/lobby/autos/carro-rojo-2.png", "sport", 90, 85, 70, 60}};
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

    carNameLabel->setStyleSheet(
        "color: #f4f4f4; background-color: rgba(0, 0, 0, 230); padding: 10px; border-radius: 5px;");
    carNameLabel->setAlignment(Qt::AlignCenter);
    carNameLabel->setGeometry(188, 100, 300, 50);

    // Imagen del auto
    carImageLabel = new QLabel(this);
    carImageLabel->setStyleSheet(
        "background-color: rgba(0, 0, 0, 155); border: 3px solid white; border-radius: 10px;");
    carImageLabel->setGeometry(210, 170, 250, 214);
    carImageLabel->setScaledContents(true);
    carImageLabel->setAlignment(Qt::AlignCenter);

    // panel de las stats
    statsPanel = new QWidget(this);
    statsPanel->setStyleSheet(
        "background-color: rgba(0, 0, 0, 230); border: 2px solid white; border-radius: 5px;");
    statsPanel->setGeometry(50, 400, 600, 150);
    createStatLabels();

    // Botón anterior
    prevButton = new QPushButton("◄", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(20);
        prevButton->setFont(btnFont);
    }
    prevButton->setStyleSheet("QPushButton {"
                              "   background-color: rgba(0, 0, 0, 230);"
                              "   color: white;"
                              "   border: 2px solid white;"
                              "   border-radius: 5px;"
                              "   padding: 10px;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #333333;"
                              "   border: 2px solid #00FF00;"
                              "}");
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
    nextButton->setStyleSheet("QPushButton {"
                              "   background-color: rgba(0, 0, 0, 230);"
                              "   color: white;"
                              "   border: 2px solid white;"
                              "   border-radius: 5px;"
                              "   padding: 10px;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #333333;"
                              "   border: 2px solid #00FF00;"
                              "}");
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
    selectButton->setStyleSheet("QPushButton {"
                                "   background-color: rgba(0, 0, 0, 230);"
                                "   color: white;"
                                "   border: 2px solid white;"
                                "   border-radius: 5px;"
                                "   padding: 15px;"
                                "}"
                                "QPushButton:hover {"
                                "   background-color: #006600;"
                                "   border: 2px solid #00FF00;"
                                "}");
    selectButton->setCursor(Qt::PointingHandCursor);
    selectButton->setGeometry(470, 590, 180, 60);
    connect(selectButton, &QPushButton::clicked, this, &GarageWindow::onSelectCar);

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
    backButton->setGeometry(50, 590, 180, 60);
    connect(backButton, &QPushButton::clicked, this, &GarageWindow::onBackClicked);
    setupMusicControl();
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
    if (currentCarIndex >= cars.size())
        return;

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
        if (value >= 80)
            barColor = QColor(0, 255, 0);
        else if (value >= 60)
            barColor = QColor(255, 255, 0);
        else
            barColor = QColor(255, 100, 0);

        statBarFills[i]->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
                                           .arg(barColor.red())
                                           .arg(barColor.green())
                                           .arg(barColor.blue()));

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
    std::cout << "Auto seleccionado: " << cars[currentCarIndex].name.toStdString() << " ("
              << cars[currentCarIndex].type.toStdString() << ")" << std::endl;
    emit carSelected(cars[currentCarIndex]);
}

void GarageWindow::onBackClicked() {
    emit backRequested();
}

GarageWindow::~GarageWindow() {}
