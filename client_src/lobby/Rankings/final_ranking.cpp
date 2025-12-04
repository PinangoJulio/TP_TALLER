
#include "final_ranking.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>

FinalRankingWindow::FinalRankingWindow(QWidget* parent) : BaseLobby(parent), customFontId(-1) {
    setWindowTitle("Need for Speed 2D - Resultados Finales");
    setFixedSize(700, 700);

    
    customFontId = QFontDatabase::addApplicationFont("assets/fonts/arcade-classic.ttf");

    backgroundImage.load("assets/img/misc/sunset.png");
    if (!backgroundImage.isNull()) {
        backgroundImage =
            backgroundImage.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    setupUI();
}

FinalRankingWindow::~FinalRankingWindow() {}

void FinalRankingWindow::setupUI() {
    QFont customFont;
    if (customFontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(customFontId);
        if (!fontFamilies.isEmpty()) {
            customFont = QFont(fontFamilies.at(0));
        }
    }

    
    titleLabel = new QLabel("RESULTADOS  FINALES", this);
    if (customFontId != -1) {
        QFont titleFont = customFont;
        titleFont.setPointSize(36);
        titleLabel->setFont(titleFont);
    }
    titleLabel->setStyleSheet("color: #ffffff; "
                              "background-color: rgba(0, 0, 0, 200); "
                              "padding: 10px; "
                              "border: 2px solid #FFD700; "
                              "border-radius: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setGeometry(100, 40, 500, 70);

    
    scrollArea = new QScrollArea(this);
    scrollArea->setGeometry(50, 130, 600, 450);
    scrollArea->setWidgetResizable(true);
    scrollArea->viewport()->setStyleSheet("background: transparent;");

    
    scrollArea->setStyleSheet(
        "QScrollArea { background: transparent; border: none; }"
        "QScrollBar:vertical {"
        "   border: none;"
        "   background: rgba(0, 0, 0, 150);"
        "   width: 10px;"
        "   margin: 0px;"
        "   border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #FFD700;"
        "   min-height: 20px;"
        "   border-radius: 5px;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");

    
    resultsContainer = new QWidget();
    resultsContainer->setStyleSheet("background: transparent;");
    resultsLayout = new QVBoxLayout(resultsContainer);
    resultsLayout->setAlignment(Qt::AlignTop);
    resultsLayout->setSpacing(10);
    resultsLayout->setContentsMargins(0, 0, 15, 0);

    scrollArea->setWidget(resultsContainer);

    
    backButton = new QPushButton("Salir del Juego", this);
    if (customFontId != -1) {
        QFont btnFont = customFont;
        btnFont.setPointSize(16);
        backButton->setFont(btnFont);
    }
    backButton->setStyleSheet("QPushButton {"
                              "   background-color: rgba(0, 0, 0, 230);"
                              "   color: white;"
                              "   border: 3px solid white;"
                              "   border-radius: 5px;"
                              "   padding: 15px;"
                              "}"
                              "QPushButton:hover {"
                              "   background-color: #333333;"
                              "   border: 3px solid #00FF00;"
                              "}");
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setGeometry(200, 610, 300, 70);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        
        QRect currentPos = this->geometry();

        
        QLabel* finalOverlay = new QLabel(nullptr);

        
        finalOverlay->setWindowFlags(Qt::Window | Qt::FramelessWindowHint |
                                     Qt::WindowStaysOnTopHint);
        finalOverlay->setFixedSize(700, 700);
        finalOverlay->setStyleSheet("background-color: black;");
        finalOverlay->setAlignment(Qt::AlignCenter);

        
        finalOverlay->move(currentPos.topLeft());

        
        QPixmap finalPix("assets/img/misc/final.png");  

        if (!finalPix.isNull()) {
            
            finalOverlay->setPixmap(
                finalPix.scaled(700, 700, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        } else {
            
            finalOverlay->setText("GRACIAS POR JUGAR");
            finalOverlay->setStyleSheet(
                "font-size: 40px; color: white; background-color: black; border: 2px solid white;");
        }

        
        this->hide();
        finalOverlay->show();

        
        QTimer::singleShot(5000, [finalOverlay]() {
            finalOverlay->close();
            delete finalOverlay;   
            QApplication::quit();  
        });
    });

    setupMusicControl();
}

QWidget* FinalRankingWindow::createRankingCard(const PlayerResult& result) {
    QWidget* card = new QWidget();
    card->setFixedHeight(80);

    
    QString borderColor = "white";
    QString rankColor = "white";
    QString bgColor = "rgba(0, 0, 0, 220)";

    if (result.rank == 1) {
        borderColor = "#FFD700";  
        rankColor = "#FFD700";
        bgColor = "rgba(50, 40, 0, 220)";
    } else if (result.rank == 2) {
        borderColor = "#D4D4D4";  
        rankColor = "#D4D4D4";
        bgColor = "rgba(73, 73, 74, 220)";
    } else if (result.rank == 3) {
        borderColor = "#CD7F32";  
        rankColor = "#CD7F32";
        bgColor = "rgba(75, 43, 10, 220)";
    }

    card->setStyleSheet(QString("QWidget { "
                                "   background-color: %1; "
                                "   border: 2px solid %2; "
                                "   border-radius: 10px; "
                                "}")
                            .arg(bgColor, borderColor));

    QHBoxLayout* layout = new QHBoxLayout(card);
    layout->setContentsMargins(20, 10, 20, 10);

    QFont font;
    if (customFontId != -1) {
        font = QFont(QFontDatabase::applicationFontFamilies(customFontId).at(0));
    }

    
    QLabel* rankLbl = new QLabel(QString("#%1").arg(result.rank));
    font.setPointSize(28);
    rankLbl->setFont(font);
    rankLbl->setStyleSheet(
        QString("color: %1; border: none; background: transparent;").arg(rankColor));
    rankLbl->setFixedWidth(60);

    
    QWidget* infoWidget = new QWidget();
    infoWidget->setStyleSheet("border: none; background: transparent;");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(2);

    QLabel* nameLbl = new QLabel(result.playerName);
    font.setPointSize(18);
    nameLbl->setFont(font);
    nameLbl->setStyleSheet("color: white;");

    QLabel* carLbl = new QLabel(result.carName);
    font.setPointSize(12);
    carLbl->setFont(font);
    carLbl->setStyleSheet("color: #D4D4D4;");

    infoLayout->addWidget(nameLbl);
    infoLayout->addWidget(carLbl);

    
    QLabel* timeLbl = new QLabel(result.totalTime);
    font.setPointSize(20);
    timeLbl->setFont(font);
    timeLbl->setStyleSheet("color: #00FF00; border: none; background: transparent;");
    timeLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(rankLbl);
    layout->addWidget(infoWidget, 1);
    layout->addWidget(timeLbl);

    return card;
}

void FinalRankingWindow::setResults(const std::vector<PlayerResult>& results) {
    QLayoutItem* item;
    while ((item = resultsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    for (const auto& player : results) {
        QWidget* card = createRankingCard(player);
        resultsLayout->addWidget(card);
    }
    resultsLayout->addStretch();
}

void FinalRankingWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    if (!backgroundImage.isNull()) {
        painter.drawPixmap(0, 0, backgroundImage);
    } else {
        QLinearGradient gradient(0, 0, 0, height());
        gradient.setColorAt(0, QColor(20, 20, 60));
        gradient.setColorAt(1, QColor(10, 10, 30));
        painter.fillRect(rect(), gradient);
    }
}
