#include "create_match_window.h"
#include <iostream>
#include <QPainter>
#include <QFontDatabase>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>

CreateMatchWindow::CreateMatchWindow(QWidget *parent)
    : BaseLobby(parent),
      hideTimer(nullptr),          
      isSelectingTrack(false),     
      backgroundImage(),           
      customFontId(-1),            
      currentCityIndex(0),         
      currentTrackIndex(0),        
      currentEditingSlot(-1),      
      totalRaces(0)                
{
    setMouseTracking(true);
    setWindowTitle("Need for Speed 2D - Crear Partida");
    setFixedSize(700, 700);

    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");

    backgroundImage.load("assets/img/lobby/window_covers/race.png");
    if (!backgroundImage.isNull())
    {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    loadCities();

    stepsStack = new QStackedWidget(this);
    stepsStack->setGeometry(0, 0, 700, 700);

    setupStep1UI();
    setupStep2UI();
    setupStep3UI();

    hideTimer = new QTimer(this);
    hideTimer->setInterval(3000); 
    connect(hideTimer, &QTimer::timeout, this, &CreateMatchWindow::hideButtons);
    stepsStack->setCurrentIndex(0);
}

void CreateMatchWindow::loadCities()
{
    // Cargar información de las 3 ciudades
    CityInfo liberty;
    liberty.name = "Liberty City";
    liberty.imagePath = "assets/img/lobby/cities/liberty-city.png";
    liberty.trackNames = {"Montline Parkway", "Upper West Side", "Lower East Side"};
    liberty.trackImagePaths = {
        "assets/img/map/cities/caminos/liberty-city/ruta-1/debug_resultado_v5.jpg",
        "assets/img/map/cities/caminos/liberty-city/ruta-2/debug_resultado_v5.png",
        "assets/img/map/cities/caminos/liberty-city/ruta-3/debug_resultado_v5.png"};
    cities.push_back(liberty);

    CityInfo sanAndreas;
    sanAndreas.name = "San Andreas";
    sanAndreas.imagePath = "assets/img/lobby/cities/san-andreas.png";
    sanAndreas.trackNames = {"Southshore District", "Vinecrest Heights", "Sunset Diagonal"};
    sanAndreas.trackImagePaths = {
        "assets/img/map/cities/caminos/san-andreas/ruta-1/debug_resultado_v5.png",
        "assets/img/map/cities/caminos/san-andreas/ruta-2/debug_resultado_v5.png",
        "assets/img/map/cities/caminos/san-andreas/ruta-3/debug_resultado_v5.jpg"};
    cities.push_back(sanAndreas);

    CityInfo viceCity;
    viceCity.name = "Vice City";
    viceCity.imagePath = "assets/img/lobby/cities/vice-city.png";
    viceCity.trackNames = {"North Grove Ribbon", "Tropical U", "Coral Serpent Way"};
    viceCity.trackImagePaths = {
        "assets/img/map/cities/caminos/vice-city/ruta-1/debug_resultado_v5.jpg",
        "assets/img/map/cities/caminos/vice-city/ruta-2/debug_resultado_v5.jpg",
        "assets/img/map/cities/caminos/vice-city/ruta-3/debug_resultado_v5.jpg"};
    cities.push_back(viceCity);
}

void CreateMatchWindow::hideButtons() {
   
   if (stepsStack->currentIndex() == 2) {
        prevCityButton->hide();
        nextCityButton->hide();
        
        editingLabel->hide();      
        cityNameLabel->hide();     
        confirmSelectionButton->hide();
        backButtonStep3->hide();   
        
        hideTimer->stop(); 
    }
}


void CreateMatchWindow::mouseMoveEvent(QMouseEvent *event) {
    if (stepsStack->currentIndex() == 2) {
        
       
        prevCityButton->show();
        nextCityButton->show();
        
        editingLabel->show();      
        cityNameLabel->show();     
        confirmSelectionButton->show(); 
        backButtonStep3->show();   

      
        hideTimer->stop();
        if (isSelectingTrack) {
            hideTimer->setInterval(5000); // 5 segundos para camino
        
        } else {
            hideTimer->setInterval(3000); // 3 segundos para ciudad
        }
        hideTimer->start();
    }
    QWidget::mouseMoveEvent(event);
}

void CreateMatchWindow::leaveEvent(QEvent *event) {

    if (stepsStack->currentIndex() == 2) {
        hideTimer->stop();
        hideTimer->start(1500); // 
    }
    QWidget::leaveEvent(event);
}



void CreateMatchWindow::enterEvent(QEnterEvent *event) { 
    if (stepsStack->currentIndex() == 2) {
        // Mostrar todos los elementos inmediatamente
        prevCityButton->show();
        nextCityButton->show();
        
        editingLabel->show();      
        cityNameLabel->show();     
        confirmSelectionButton->show(); 
        backButtonStep3->show();   
        
        // Reiniciar el timer 
        hideTimer->stop();
        if (isSelectingTrack) {
            hideTimer->setInterval(5000); // 5 segundos para camino
        } else {
            hideTimer->setInterval(3000); // 3 segundos para ciudad
        }
        hideTimer->start(); 
    }
    QWidget::enterEvent(event);
}

void CreateMatchWindow::setupStep1UI()
{
    step1Widget = new QWidget();

    QFont customFont;
    if (customFontId != -1)
    {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty())
        {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    // Título
    step1TitleLabel = new QLabel("Crear Nueva Partida", step1Widget);
    if (customFontId != -1)
    {
        QFont titleFont = customFont;
        titleFont.setPointSize(28);
        step1TitleLabel->setFont(titleFont);
    }
    step1TitleLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 230); padding: 15px; border-radius: 5px;");
    step1TitleLabel->setAlignment(Qt::AlignCenter);
    step1TitleLabel->setGeometry(100, 80, 500, 60);

    matchNameLabel = new QLabel("Nombre de la Sala:", step1Widget);
    if (customFontId != -1)
    {
        QFont labelFont = customFont;
        labelFont.setPointSize(14);
        matchNameLabel->setFont(labelFont);
    }
    matchNameLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 180); padding: 8px; border-radius: 5px;");
    matchNameLabel->setGeometry(100, 180, 500, 35);

    matchNameInput = new QLineEdit(step1Widget);
    if (customFontId != -1)
    {
        QFont inputFont = customFont;
        inputFont.setPointSize(14);
        matchNameInput->setFont(inputFont);
    }
    matchNameInput->setStyleSheet(
        "QLineEdit {"
        "   background-color: rgba(255, 255, 255, 230);"
        "   color: black;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QLineEdit:focus {"
        "   border: 2px solid #00FF00;"
        "}");
    matchNameInput->setPlaceholderText("Ej: Carrera Nocturna");
    matchNameInput->setMaxLength(30);
    matchNameInput->setGeometry(100, 225, 500, 50);
    connect(matchNameInput, &QLineEdit::textChanged, this, &CreateMatchWindow::onMatchNameChanged);

    maxPlayersLabel = new QLabel("Jugadores Maximos (2-8):", step1Widget);
    if (customFontId != -1)
    {
        QFont labelFont = customFont;
        labelFont.setPointSize(14);
        maxPlayersLabel->setFont(labelFont);
    }
    maxPlayersLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 180); padding: 8px; border-radius: 5px;");
    maxPlayersLabel->setGeometry(100, 300, 500, 35);

    maxPlayersSpinBox = new QSpinBox(step1Widget);
    if (customFontId != -1)
    {
        QFont spinFont = customFont;
        spinFont.setPointSize(16);
        maxPlayersSpinBox->setFont(spinFont);
    }
    maxPlayersSpinBox->setStyleSheet(
        "QSpinBox {"
        "   background-color: rgba(255, 255, 255, 230);"
        "   color: black;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}");
    maxPlayersSpinBox->setMinimum(2);
    maxPlayersSpinBox->setMaximum(8);
    maxPlayersSpinBox->setValue(2);
    maxPlayersSpinBox->setGeometry(100, 345, 500, 50);

    numRacesLabel = new QLabel("Numero de Carreras:", step1Widget);
    if (customFontId != -1)
    {
        QFont labelFont = customFont;
        labelFont.setPointSize(14);
        numRacesLabel->setFont(labelFont);
    }
    numRacesLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 180); padding: 8px; border-radius: 5px;");
    numRacesLabel->setGeometry(100, 420, 500, 35);

    numRacesSpinBox = new QSpinBox(step1Widget);
    if (customFontId != -1)
    {
        QFont spinFont = customFont;
        spinFont.setPointSize(16);
        numRacesSpinBox->setFont(spinFont);
    }
    numRacesSpinBox->setStyleSheet(
        "QSpinBox {"
        "   background-color: rgba(255, 255, 255, 230);"
        "   color: black;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}");
    numRacesSpinBox->setMinimum(3);
    numRacesSpinBox->setMaximum(5);
    numRacesSpinBox->setValue(3);
    numRacesSpinBox->setGeometry(100, 465, 500, 50);

    maxPlayersSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    numRacesSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);

    nextStepButton = new QPushButton("Customizar", step1Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        nextStepButton->setFont(btnFont);
    }
    nextStepButton->setStyleSheet(
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
        "QPushButton:disabled {"
        "   background-color: #555555;"
        "   color: #888888;"
        "}");
    nextStepButton->setCursor(Qt::PointingHandCursor);
    nextStepButton->setEnabled(false);
    nextStepButton->setGeometry(470, 590, 180, 70);
    connect(nextStepButton, &QPushButton::clicked, this, &CreateMatchWindow::onNextToRaceList);

    backButtonStep1 = new QPushButton("Volver", step1Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        backButtonStep1->setFont(btnFont);
    }
    backButtonStep1->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333;"
        "   border: 2px solid #FF0000;"
        "}");
    backButtonStep1->setCursor(Qt::PointingHandCursor);
    backButtonStep1->setGeometry(50, 590, 180, 70);

    connect(backButtonStep1, &QPushButton::clicked, this, &CreateMatchWindow::backRequested);
    stepsStack->addWidget(step1Widget);
    setupMusicControl();
}

void CreateMatchWindow::setupStep2UI()
{
    step2Widget = new QWidget();

    QFont customFont;
    if (customFontId != -1)
    {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty())
        {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    step2TitleLabel = new QLabel("Configurar Carreras", step2Widget);
    if (customFontId != -1)
    {
        QFont titleFont = customFont;
        titleFont.setPointSize(24);
        step2TitleLabel->setFont(titleFont);
    }
    step2TitleLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 230); padding: 10px; border-radius: 5px;");
    step2TitleLabel->setAlignment(Qt::AlignCenter);
    step2TitleLabel->setGeometry(100, 50, 500, 50);

    instructionLabel = new QLabel("Haz click en una carrera para configurarla", step2Widget);
    if (customFontId != -1)
    {
        QFont instrFont = customFont;
        instrFont.setPointSize(12);
        instructionLabel->setFont(instrFont);
    }
    instructionLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 180); padding: 8px; border-radius: 5px;");
    instructionLabel->setAlignment(Qt::AlignCenter);
    instructionLabel->setGeometry(100, 120, 500, 35);

    // Lista de carreras
    raceListWidget = new QListWidget(step2Widget);
    if (customFontId != -1)
    {
        QFont listFont = customFont;
        listFont.setPointSize(11);
        raceListWidget->setFont(listFont);
    }
    raceListWidget->setStyleSheet(
        "QListWidget {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QListWidget::item {"
        "   padding: 15px;"
        "   border: 2px solid #666666;"
        "   border-radius: 5px;"
        "   margin: 8px;"
        "   background-color: rgba(50, 50, 50, 200);"
        "}"
        "QListWidget::item:hover {"
        "   background-color: rgba(100, 100, 100, 200);"
        "   border: 2px solid #00FF00;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: rgba(0, 150, 0, 150);"
        "   border: 2px solid #00FF00;"
        "}");
    raceListWidget->setGeometry(50, 180, 600, 380);
    connect(raceListWidget, &QListWidget::itemClicked, this, &CreateMatchWindow::onRaceSlotClicked);

    progressLabel = new QLabel("0 / 3 carreras configuradas", step2Widget);
    if (customFontId != -1)
    {
        QFont labelFont = customFont;
        labelFont.setPointSize(12);
        progressLabel->setFont(labelFont);
    }
    progressLabel->setStyleSheet("color: #FFD700; background-color: rgba(0, 0, 0, 180); padding: 5px; border-radius: 5px;");
    progressLabel->setAlignment(Qt::AlignCenter);
    progressLabel->setGeometry(150, 550, 400, 30);

    // Botones
    confirmButton = new QPushButton("Crear Partida", step2Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        confirmButton->setFont(btnFont);
    }
    confirmButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #006600;"
        "   border: 2px solid #FFD700;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #555555;"
        "   color: #888888;"
        "}");
    confirmButton->setCursor(Qt::PointingHandCursor);
    confirmButton->setEnabled(false);
    confirmButton->setGeometry(470, 590, 180, 70);
    connect(confirmButton, &QPushButton::clicked, this, &CreateMatchWindow::onConfirmRaceList);

    backButtonStep2 = new QPushButton("Volver", step2Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        backButtonStep2->setFont(btnFont);
    }
    backButtonStep2->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #333333;"
        "   border: 2px solid #FF0000;"
        "}");
    backButtonStep2->setCursor(Qt::PointingHandCursor);
    backButtonStep2->setGeometry(50, 590, 180, 70);
    connect(backButtonStep2, &QPushButton::clicked, this, &CreateMatchWindow::onBackFromRaceList);
    setupMusicControl();

    stepsStack->addWidget(step2Widget);
}

void CreateMatchWindow::setupStep3UI()
{
    step3Widget = new QWidget();

    QFont customFont;
    if (customFontId != -1)
    {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty())
        {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    // Label de qué carrera se está customizando
    editingLabel = new QLabel("Configurando: Carrera 1", step3Widget);
    if (customFontId != -1)
    {
        QFont labelFont = customFont;
        labelFont.setPointSize(14);
        editingLabel->setFont(labelFont);
    }
    editingLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 230); padding: 10px; border-radius: 5px;");
    editingLabel->setAlignment(Qt::AlignCenter);
    editingLabel->setGeometry(150, 20, 450, 40);

    // Nombre de la ciudad
    cityNameLabel = new QLabel("", step3Widget);
    if (customFontId != -1)
    {
        QFont nameFont = customFont;
        nameFont.setPointSize(28);
        cityNameLabel->setFont(nameFont);
    }
    cityNameLabel->setStyleSheet("color: white; background-color: rgba(0, 0, 0, 200); padding: 15px; border-radius: 8px;");
    cityNameLabel->setAlignment(Qt::AlignCenter);
    cityNameLabel->setGeometry(150, 80, 400, 60);

    prevCityButton = new QPushButton("◄", step3Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(30);
        prevCityButton->setFont(btnFont);
    }
    prevCityButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 200);"
        "   color: white;"
        "   border: 3px solid white;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 100, 0, 200);"
        "   border: 3px solid #00FF00;"
        "}");
    prevCityButton->setCursor(Qt::PointingHandCursor);
    prevCityButton->setGeometry(20, 300, 70, 70);
    connect(prevCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onPreviousCity);

    nextCityButton = new QPushButton("►", step3Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(30);
        nextCityButton->setFont(btnFont);
    }
    nextCityButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 200);"
        "   color: white;"
        "   border: 3px solid white;"
        "   border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 100, 0, 200);"
        "   border: 3px solid #00FF00;"
        "}");
    nextCityButton->setCursor(Qt::PointingHandCursor);
    nextCityButton->setGeometry(600, 300, 70, 70);
    connect(nextCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onNextCity);

    confirmSelectionButton = new QPushButton("Seleccionar", step3Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(16);
        confirmSelectionButton->setFont(btnFont);
    }
    confirmSelectionButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 3px solid white;"
        "   border-radius: 8px;"
        "   padding: 20px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(0, 100, 0, 230);"
        "   border: 3px solid #00FF00;"
        "}");
    confirmSelectionButton->setCursor(Qt::PointingHandCursor);
    confirmSelectionButton->setGeometry(470, 600, 180, 60);
    connect(confirmSelectionButton, &QPushButton::clicked, this, &CreateMatchWindow::onConfirmSelection);

    backButtonStep3 = new QPushButton("Cancelar", step3Widget);
    if (customFontId != -1)
    {
        QFont btnFont = customFont;
        btnFont.setPointSize(14);
        backButtonStep3->setFont(btnFont);
    }
    backButtonStep3->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(0, 0, 0, 230);"
        "   color: white;"
        "   border: 2px solid white;"
        "   border-radius: 5px;"
        "   padding: 10px;"
        "}"
        "QPushButton:hover {"
        "   background-color: rgba(100, 0, 0, 230);"
        "   border: 2px solid #FF0000;"
        "}");
    backButtonStep3->setCursor(Qt::PointingHandCursor);
    backButtonStep3->setGeometry(50, 600, 180, 60);
    connect(backButtonStep3, &QPushButton::clicked, this, &CreateMatchWindow::onBackFromSelector);

    trackSectionLabel = nullptr;
    trackNameLabel = nullptr;
    trackImageLabel = nullptr;
    prevTrackButton = nullptr;
    nextTrackButton = nullptr;
    setupMusicControl(); 

    step3Widget->setMouseTracking(true);
    editingLabel->setMouseTracking(true);
    cityNameLabel->setMouseTracking(true);
    prevCityButton->setMouseTracking(true);
    nextCityButton->setMouseTracking(true);
    confirmSelectionButton->setMouseTracking(true);
    backButtonStep3->setMouseTracking(true);

    stepsStack->addWidget(step3Widget);
}

void CreateMatchWindow::validateStep1()
{
    QString matchName = matchNameInput->text().trimmed();
    bool valid = !matchName.isEmpty() && matchName.length() >= 3;
    nextStepButton->setEnabled(valid);
}

void CreateMatchWindow::onMatchNameChanged(const QString &text)
{
    (void)text;
    validateStep1();
}

void CreateMatchWindow::onNextToRaceList()
{
    totalRaces = numRacesSpinBox->value();

    // Inicializar carreras vacías
    configuredRaces.clear();
    configuredRaces.resize(totalRaces);

    for (int i = 0; i < totalRaces; i++)
    {
        configuredRaces[i].cityName = "";
        configuredRaces[i].trackName = "";
        configuredRaces[i].trackIndex = -1;
    }

    updateRaceList();
    stepsStack->setCurrentIndex(1);
}

void CreateMatchWindow::updateRaceList()
{
    raceListWidget->clear();

    int configuredCount = 0;

    for (size_t i = 0; i < configuredRaces.size(); i++)
    {
        const RaceConfig &race = configuredRaces[i];
        QString itemText;

        if (race.cityName.isEmpty())
        {
            itemText = QString("Carrera %1: [Click para configurar]").arg(i + 1);
        }
        else
        {
            itemText = QString("Carrera %1: %2 - %3 ✓")
                           .arg(i + 1)
                           .arg(race.cityName)
                           .arg(race.trackName);
            configuredCount++;
        }

        QListWidgetItem *item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, static_cast<int>(i)); // Guardar índice
        raceListWidget->addItem(item);
    }

    progressLabel->setText(QString("%1 / %2 carreras configuradas")
                               .arg(configuredCount)
                               .arg(totalRaces));

    // Habilitar botón de confirmar si todas las carreras están configuradas
    confirmButton->setEnabled(configuredCount == totalRaces);
}

void CreateMatchWindow::onRaceSlotClicked(QListWidgetItem *item)
{
    if (!item)
        return;

    currentEditingSlot = item->data(Qt::UserRole).toInt();

    // Si ya tiene configuración, cargar esos datos
    const RaceConfig &race = configuredRaces[currentEditingSlot];
    if (!race.cityName.isEmpty())
    {
        // Encontrar el índice de la ciudad
        for (size_t i = 0; i < cities.size(); i++)
        {
            if (cities[i].name == race.cityName)
            {
                currentCityIndex = i;
                currentTrackIndex = race.trackIndex;
                break;
            }
        }
    }
    else
    {
        // Iniciar desde el principio
        currentCityIndex = 0;
        currentTrackIndex = 0;
    }

    // Estamos en modo selección de ciudad
    isSelectingTrack = false;

    editingLabel->setText(QString("Configurando: Carrera %1 - Selecciona Ciudad").arg(currentEditingSlot + 1));

    // Cambiar texto del botón para seleccionar ciudad
    confirmSelectionButton->setText("Seleccionar");
    confirmSelectionButton->disconnect();
    connect(confirmSelectionButton, &QPushButton::clicked, this, &CreateMatchWindow::onCitySelected);

    updateCityDisplay();
    stepsStack->setCurrentIndex(2);
    
    // Mostrar todos los botones inicialmente
    prevCityButton->show();
    nextCityButton->show();
    editingLabel->show();      
    cityNameLabel->show();     
    confirmSelectionButton->show(); 
    backButtonStep3->show();   
    
    // Iniciar el temporizador con intervalo para ciudad (3 segundos)
    hideTimer->stop();
    hideTimer->setInterval(3000);
    hideTimer->start();
}

void CreateMatchWindow::updateCityDisplay()
{
    if (currentCityIndex >= cities.size())
        return;

    const CityInfo &city = cities[currentCityIndex];
    cityNameLabel->setText(city.name);

    // Cargar imagen de la ciudad como fondo
    QPixmap cityImage(city.imagePath);
    if (!cityImage.isNull())
    {
        backgroundImage = cityImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        // Placeholder si no existe
        backgroundImage = QPixmap(700, 700);
        backgroundImage.fill(QColor(30, 30, 30));
        QPainter p(&backgroundImage);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 32));
        p.drawText(backgroundImage.rect(), Qt::AlignCenter, city.name);
    }

    step3Widget->update();
}

void CreateMatchWindow::updateTrackDisplay()
{
    if (currentCityIndex >= cities.size())
        return;

    const CityInfo &city = cities[currentCityIndex];
    if (currentTrackIndex < 0 || currentTrackIndex >= static_cast<int>(city.trackNames.size()))
        return;

    // Cargar imagen del camino como fondo
    QPixmap trackImage(city.trackImagePaths[currentTrackIndex]);
    if (!trackImage.isNull())
    {
        backgroundImage = trackImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    else
    {
        // Placeholder si no existe
        backgroundImage = QPixmap(700, 700);
        backgroundImage.fill(QColor(40, 40, 40));
        QPainter p(&backgroundImage);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 24));
        p.drawText(backgroundImage.rect(), Qt::AlignCenter,
                   city.name + "\n" + city.trackNames[currentTrackIndex]);
    }

    cityNameLabel->setText(city.trackNames[currentTrackIndex]);
    step3Widget->update(); // Forzar repintado
}

void CreateMatchWindow::onPreviousCity()
{
    if (currentCityIndex == 0)
    {
        currentCityIndex = cities.size() - 1;
    }
    else
    {
        currentCityIndex--;
    }
    updateCityDisplay();
}

void CreateMatchWindow::onNextCity()
{
    currentCityIndex++;
    if (currentCityIndex >= cities.size())
    {
        currentCityIndex = 0;
    }
    updateCityDisplay();
}

void CreateMatchWindow::onCitySelected()
{

    if (currentCityIndex >= cities.size())
        return;

    currentTrackIndex = 0;

    // Ahora estamos en modo selección de camino
    isSelectingTrack = true;

    editingLabel->setText(QString("Configurando: Carrera %1 - Selecciona Recorrido").arg(currentEditingSlot + 1));

    confirmSelectionButton->setText("Confirmar");
    confirmSelectionButton->disconnect();
    connect(confirmSelectionButton, &QPushButton::clicked, this, &CreateMatchWindow::onConfirmSelection);

    prevCityButton->disconnect();
    nextCityButton->disconnect();
    connect(prevCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onPreviousTrack);
    connect(nextCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onNextTrack);

    updateTrackDisplay();
    
    // Mostrar botones y reiniciar timer con intervalo para cmino (5 segundos)
    prevCityButton->show();
    nextCityButton->show();
    editingLabel->show();      
    cityNameLabel->show();     
    confirmSelectionButton->show(); 
    backButtonStep3->show();
    
    hideTimer->stop();
    hideTimer->setInterval(5000); // 5 segundos para mirar el mapa
    hideTimer->start();
}

void CreateMatchWindow::onPreviousTrack()
{
    if (currentCityIndex >= cities.size())
        return;

    const CityInfo &city = cities[currentCityIndex];
    if (currentTrackIndex == 0)
    {
        currentTrackIndex = city.trackNames.size() - 1;
    }
    else
    {
        currentTrackIndex--;
    }
    updateTrackDisplay();
}

void CreateMatchWindow::onNextTrack()
{
    if (currentCityIndex >= cities.size())
        return;

    const CityInfo &city = cities[currentCityIndex];
    currentTrackIndex++;
    if (currentTrackIndex >= static_cast<int>(city.trackNames.size()))
    {
        currentTrackIndex = 0;
    }
    updateTrackDisplay();
}

void CreateMatchWindow::onConfirmSelection()
{
    if (currentEditingSlot < 0 || currentEditingSlot >= static_cast<int>(configuredRaces.size()))
        return;
    if (currentCityIndex >= cities.size())
        return;

    const CityInfo &city = cities[currentCityIndex];
    if (currentTrackIndex < 0 || currentTrackIndex >= static_cast<int>(city.trackNames.size()))
        return;

    // Guardar la configuración
    RaceConfig &race = configuredRaces[currentEditingSlot];
    race.cityName = city.name;
    race.trackIndex = currentTrackIndex;
    race.trackName = city.trackNames[currentTrackIndex];

    std::cout << "Carrera " << (currentEditingSlot + 1) << " configurada: "
              << race.cityName.toStdString() << " - " << race.trackName.toStdString() << std::endl;

    // Restaurar estado inicial para próxima selección
    isSelectingTrack = false;
    
    prevCityButton->disconnect();
    nextCityButton->disconnect();
    connect(prevCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onPreviousCity);
    connect(nextCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onNextCity);

    backgroundImage.load("assets/img/lobby/window_covers/race.png");
    if (!backgroundImage.isNull())
    {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    updateRaceList();
    stepsStack->setCurrentIndex(1);
}

void CreateMatchWindow::onBackFromSelector()
{
    // Restaurar estado
    isSelectingTrack = false;

    prevCityButton->disconnect();
    nextCityButton->disconnect();
    connect(prevCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onPreviousCity);
    connect(nextCityButton, &QPushButton::clicked, this, &CreateMatchWindow::onNextCity);

    backgroundImage.load("assets/img/lobby/window_covers/race.png");
    if (!backgroundImage.isNull())
    {
        backgroundImage = backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    stepsStack->setCurrentIndex(1);
}

void CreateMatchWindow::onConfirmRaceList()
{
    QString matchName = matchNameInput->text().trimmed();
    int maxPlayers = maxPlayersSpinBox->value();

    std::cout << "Confirmando creación de partida:" << std::endl;
    std::cout << "  Nombre: " << matchName.toStdString() << std::endl;
    std::cout << "  Jugadores: " << maxPlayers << std::endl;
    std::cout << "  Carreras: " << configuredRaces.size() << std::endl;

    for (size_t i = 0; i < configuredRaces.size(); i++)
    {
        std::cout << "  Carrera " << (i + 1) << ": "
                  << configuredRaces[i].cityName.toStdString()
                  << " - " << configuredRaces[i].trackName.toStdString() << std::endl;
    }

    emit matchCreated(matchName, maxPlayers, configuredRaces);
}

void CreateMatchWindow::onBackFromRaceList()
{
    // Volver al paso 1
    stepsStack->setCurrentIndex(0);
}

void CreateMatchWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);

    // Dibujar la imagen de fondo actual
    if (!backgroundImage.isNull())
    {
        painter.drawPixmap(0, 0, backgroundImage);
    }
    else
    {
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor(20, 20, 60));
        gradient.setColorAt(1, QColor(10, 10, 30));
        painter.fillRect(rect(), gradient);
    }

    QWidget::paintEvent(event);
}

CreateMatchWindow::~CreateMatchWindow()
{
}