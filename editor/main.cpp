#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QGroupBox>
#include <QScrollArea>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QPainter>
#include <QDir>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

struct SpawnPoint {
    double x;
    double y;
    double angle;
};

struct Checkpoint {
    int id;
    std::string type;
    double x;
    double y;
    double width;
    double height;
    double angle;
};

// Vista interactiva del mapa
class MapView : public QGraphicsView {
    Q_OBJECT

private:
    QGraphicsScene* scene;
    QGraphicsPixmapItem* mapItem;
    std::vector<QGraphicsEllipseItem*> checkpointMarkers;
    std::vector<QGraphicsTextItem*> checkpointLabels;
    std::vector<QGraphicsEllipseItem*> spawnMarkers;

    enum class EditMode {
        None,
        PlaceCheckpoint,
        PlaceSpawn
    };

    EditMode currentMode = EditMode::None;
    QString checkpointType = "normal";

public:
    MapView(QWidget* parent = nullptr) : QGraphicsView(parent) {
        scene = new QGraphicsScene(this);
        setScene(scene);
        mapItem = nullptr;

        setDragMode(QGraphicsView::ScrollHandDrag);
        setRenderHint(QPainter::Antialiasing);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

        // Permitir zoom con rueda del mouse
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }

    void loadMap(const QString& imagePath) {
        scene->clear();
        checkpointMarkers.clear();
        checkpointLabels.clear();
        spawnMarkers.clear();

        QPixmap map(imagePath);
        if (!map.isNull()) {
            mapItem = scene->addPixmap(map);
            setSceneRect(map.rect());
        }
    }

    void clearMarkers() {
        for (auto marker : checkpointMarkers) {
            scene->removeItem(marker);
            delete marker;
        }
        for (auto label : checkpointLabels) {
            scene->removeItem(label);
            delete label;
        }
        for (auto marker : spawnMarkers) {
            scene->removeItem(marker);
            delete marker;
        }
        checkpointMarkers.clear();
        checkpointLabels.clear();
        spawnMarkers.clear();
    }

    void displayCheckpoints(const std::vector<Checkpoint>& checkpoints) {
        clearMarkers();

        for (const auto& cp : checkpoints) {
            QColor color;
            if (cp.type == "start") color = Qt::green;
            else if (cp.type == "finish") color = Qt::red;
            else color = Qt::yellow;

            auto marker = scene->addEllipse(cp.x - 5, cp.y - 5, 10, 10,
                QPen(Qt::black, 2), QBrush(color));
            checkpointMarkers.push_back(marker);

            auto label = scene->addText(QString::number(cp.id));
            label->setPos(cp.x + 8, cp.y - 8);
            label->setDefaultTextColor(Qt::white);
            checkpointLabels.push_back(label);
        }
    }

    void displaySpawnPoints(const std::vector<SpawnPoint>& spawns) {
        for (auto marker : spawnMarkers) {
            scene->removeItem(marker);
            delete marker;
        }
        spawnMarkers.clear();

        for (const auto& sp : spawns) {
            auto marker = scene->addEllipse(sp.x - 4, sp.y - 4, 8, 8,
                QPen(Qt::black, 1), QBrush(Qt::blue));
            spawnMarkers.push_back(marker);
        }
    }

    void setCheckpointMode(const QString& type) {
        currentMode = EditMode::PlaceCheckpoint;
        checkpointType = type;
        setDragMode(QGraphicsView::NoDrag);
    }

    void setSpawnMode() {
        currentMode = EditMode::PlaceSpawn;
        setDragMode(QGraphicsView::NoDrag);
    }

    void setNavigationMode() {
        currentMode = EditMode::None;
        setDragMode(QGraphicsView::ScrollHandDrag);
    }

signals:
    void checkpointPlaced(double x, double y, QString type);
    void spawnPlaced(double x, double y);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (currentMode == EditMode::None) {
            QGraphicsView::mousePressEvent(event);
            return;
        }

        QPointF scenePos = mapToScene(event->pos());

        if (currentMode == EditMode::PlaceCheckpoint) {
            emit checkpointPlaced(scenePos.x(), scenePos.y(), checkpointType);
        } else if (currentMode == EditMode::PlaceSpawn) {
            emit spawnPlaced(scenePos.x(), scenePos.y());
        }
    }

    void wheelEvent(QWheelEvent* event) override {
        if (event->modifiers() & Qt::ControlModifier) {
            double scaleFactor = 1.15;
            if (event->angleDelta().y() > 0) {
                scale(scaleFactor, scaleFactor);
            } else {
                scale(1.0 / scaleFactor, 1.0 / scaleFactor);
            }
            event->accept();
        } else {
            QGraphicsView::wheelEvent(event);
        }
    }
};

class MapEditorWindow : public QMainWindow {
    Q_OBJECT

private:
    // Información de la ruta
    QComboBox* cityCombo;
    QComboBox* routeCombo;
    QLabel* infoLabel;

    // Vista del mapa
    MapView* mapView;

    // Controles
    QPushButton* loadButton;
    QPushButton* saveButton;
    QPushButton* addStartBtn;
    QPushButton* addNormalBtn;
    QPushButton* addFinishBtn;
    QPushButton* addSpawnBtn;
    QPushButton* deleteLastCheckpointBtn;
    QPushButton* deleteLastSpawnBtn;
    QPushButton* clearAllBtn;

    // Datos
    std::vector<SpawnPoint> spawnPoints;
    std::vector<Checkpoint> checkpoints;
    QString currentCity;
    QString currentRoute;
    QString raceDescription;
    int maxTimeSeconds = 600;
    int totalLaps = 3;

    void setupUI() {
        setWindowTitle("Editor de Rutas - Need for Speed 2D");
        resize(1400, 900);

        QWidget* centralWidget = new QWidget(this);
        QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);

        // Panel izquierdo (controles)
        QVBoxLayout* leftPanel = new QVBoxLayout();

        // Selector de ciudad y ruta
        QGroupBox* selectionGroup = new QGroupBox("Selección de Ruta");
        QVBoxLayout* selectionLayout = new QVBoxLayout();

        QLabel* cityLabel = new QLabel("Ciudad:");
        cityCombo = new QComboBox();
        cityCombo->addItem("Liberty City");
        cityCombo->addItem("San Andreas");
        cityCombo->addItem("Vice City");

        QLabel* routeLabel = new QLabel("Ruta:");
        routeCombo = new QComboBox();
        routeCombo->addItem("ruta-1");
        routeCombo->addItem("ruta-2");
        routeCombo->addItem("ruta-3");

        loadButton = new QPushButton("Cargar Ruta (Crear Nueva)");
        loadButton->setStyleSheet("background-color: #2196F3; color: white; font-weight: bold;");

        QPushButton* viewExistingBtn = new QPushButton("Ver Ruta Actual (Referencia)");
        viewExistingBtn->setStyleSheet("background-color: #9E9E9E; color: white;");

        saveButton = new QPushButton("Guardar Cambios");
        saveButton->setStyleSheet("background-color: #4CAF50; color: white; font-weight: bold;");

        selectionLayout->addWidget(cityLabel);
        selectionLayout->addWidget(cityCombo);
        selectionLayout->addWidget(routeLabel);
        selectionLayout->addWidget(routeCombo);
        selectionLayout->addWidget(loadButton);
        selectionLayout->addWidget(viewExistingBtn);
        selectionLayout->addWidget(saveButton);
        selectionGroup->setLayout(selectionLayout);

        // Controles de edición
        QGroupBox* editGroup = new QGroupBox("Herramientas de Edición");
        QVBoxLayout* editLayout = new QVBoxLayout();

        QLabel* checkpointLabel = new QLabel("<b>Checkpoints:</b>");
        addStartBtn = new QPushButton("Colocar START (Verde)");
        addStartBtn->setStyleSheet("background-color: green; color: white;");

        addNormalBtn = new QPushButton("Colocar NORMAL (Amarillo)");
        addNormalBtn->setStyleSheet("background-color: orange; color: white;");

        addFinishBtn = new QPushButton("Colocar FINISH (Rojo)");
        addFinishBtn->setStyleSheet("background-color: red; color: white;");

        deleteLastCheckpointBtn = new QPushButton("Eliminar Último Checkpoint");

        QLabel* spawnLabel = new QLabel("<b>Spawn Points:</b>");
        addSpawnBtn = new QPushButton("Colocar Spawn Point (Azul)");
        addSpawnBtn->setStyleSheet("background-color: blue; color: white;");

        deleteLastSpawnBtn = new QPushButton("Eliminar Último Spawn");

        clearAllBtn = new QPushButton("Limpiar Todo");
        clearAllBtn->setStyleSheet("background-color: darkred; color: white;");

        editLayout->addWidget(checkpointLabel);
        editLayout->addWidget(addStartBtn);
        editLayout->addWidget(addNormalBtn);
        editLayout->addWidget(addFinishBtn);
        editLayout->addWidget(deleteLastCheckpointBtn);
        editLayout->addSpacing(20);
        editLayout->addWidget(spawnLabel);
        editLayout->addWidget(addSpawnBtn);
        editLayout->addWidget(deleteLastSpawnBtn);
        editLayout->addSpacing(20);
        editLayout->addWidget(clearAllBtn);
        editLayout->addStretch();
        editGroup->setLayout(editLayout);

        // Info
        infoLabel = new QLabel("Carga una ruta para comenzar");
        infoLabel->setWordWrap(true);
        infoLabel->setStyleSheet("background-color: #f0f0f0; padding: 10px; border-radius: 5px;");

        leftPanel->addWidget(selectionGroup);
        leftPanel->addWidget(editGroup);
        leftPanel->addWidget(new QLabel("<b>Información:</b>"));
        leftPanel->addWidget(infoLabel);
        leftPanel->addStretch();

        // Panel derecho (vista del mapa)
        QVBoxLayout* rightPanel = new QVBoxLayout();
        QLabel* mapLabel = new QLabel("<b>Mapa de la Ruta</b>");
        mapLabel->setAlignment(Qt::AlignCenter);

        mapView = new MapView();
        mapView->setMinimumSize(800, 600);

        QLabel* helpLabel = new QLabel(
            "💡 <b>Ayuda:</b><br>"
            "• Click en el mapa para colocar checkpoints/spawns<br>"
            "• Ctrl + Rueda: Zoom<br>"
            "• Arrastrar: Mover vista<br>"
            "• Los checkpoints se numeran automáticamente"
        );
        helpLabel->setStyleSheet("background-color: #e3f2fd; padding: 8px; border-radius: 5px;");

        rightPanel->addWidget(mapLabel);
        rightPanel->addWidget(mapView);
        rightPanel->addWidget(helpLabel);

        mainLayout->addLayout(leftPanel, 1);
        mainLayout->addLayout(rightPanel, 3);

        setCentralWidget(centralWidget);

        // Conexiones
        connect(loadButton, &QPushButton::clicked, this, &MapEditorWindow::loadRoute);
        connect(viewExistingBtn, &QPushButton::clicked, this, &MapEditorWindow::viewExistingRoute);
        connect(saveButton, &QPushButton::clicked, this, &MapEditorWindow::saveRoute);
        connect(addStartBtn, &QPushButton::clicked, this, [this]() {
            mapView->setCheckpointMode("start");
        });
        connect(addNormalBtn, &QPushButton::clicked, this, [this]() {
            mapView->setCheckpointMode("normal");
        });
        connect(addFinishBtn, &QPushButton::clicked, this, [this]() {
            mapView->setCheckpointMode("finish");
        });
        connect(addSpawnBtn, &QPushButton::clicked, this, [this]() {
            mapView->setSpawnMode();
        });
        connect(deleteLastCheckpointBtn, &QPushButton::clicked, this, &MapEditorWindow::deleteLastCheckpoint);
        connect(deleteLastSpawnBtn, &QPushButton::clicked, this, &MapEditorWindow::deleteLastSpawn);
        connect(clearAllBtn, &QPushButton::clicked, this, &MapEditorWindow::clearAll);

        connect(mapView, &MapView::checkpointPlaced, this, &MapEditorWindow::onCheckpointPlaced);
        connect(mapView, &MapView::spawnPlaced, this, &MapEditorWindow::onSpawnPlaced);
    }

    QString getCityFolderName(const QString& city) {
        if (city == "Liberty City") return "Liberty City";
        if (city == "San Andreas") return "San Andreas";
        if (city == "Vice City") return "Vice City";
        return city;
    }

    QString getCityImageFolderName(const QString& city) {
        if (city == "Liberty City") return "liberty-city";
        if (city == "San Andreas") return "san-andreas";
        if (city == "Vice City") return "vice-city";
        return city.toLower();
    }

private slots:
    void loadRoute() {
        currentCity = cityCombo->currentText();
        currentRoute = routeCombo->currentText();

        QString cityFolder = getCityFolderName(currentCity);
        QString yamlPath = QString("server_src/city_maps/%1/%2.yaml")
            .arg(cityFolder).arg(currentRoute);

        try {
            YAML::Node config = YAML::LoadFile(yamlPath.toStdString());

            // Cargar solo la información básica de la ruta
            raceDescription = QString::fromStdString(config["race"]["description"].as<std::string>());
            maxTimeSeconds = config["race"]["max_time_seconds"].as<int>();
            totalLaps = config["race"]["total_laps"].as<int>();

            // IMPORTANTE: Iniciar VACÍO para crear nuevo camino
            // El usuario creará su propio camino desde cero
            spawnPoints.clear();
            checkpoints.clear();

            // Cargar imagen del mapa
            QString cityImgFolder = getCityImageFolderName(currentCity);

            // Intentar diferentes nombres de imagen en orden de prioridad
            QStringList possibleImages = {
                QString("assets/img/map/cities/caminos/%1/%2/debug_resultado_v5.png").arg(cityImgFolder).arg(currentRoute),
                QString("assets/img/map/cities/caminos/%1/%2/%3.png").arg(cityImgFolder).arg(currentRoute).arg(cityImgFolder + "1"),
                QString("assets/img/map/cities/caminos/%1/%2/map.png").arg(cityImgFolder).arg(currentRoute),
                QString("assets/img/map/cities/%1.png").arg(cityImgFolder)
            };

            QString imagePath;
            for (const QString& path : possibleImages) {
                if (QFile::exists(path)) {
                    imagePath = path;
                    break;
                }
            }

            if (imagePath.isEmpty()) {
                QMessageBox::warning(this, "Advertencia",
                    QString("No se encontró imagen del mapa.\nBuscado en: %1").arg(possibleImages[0]));
            }

            mapView->loadMap(imagePath);
            mapView->clearMarkers();  // Empezar con mapa vacío

            updateInfo();

            QMessageBox::information(this, "Ruta Cargada",
                QString("<b>%1 - %2</b><br><br>"
                        "El mapa ha sido cargado.<br>"
                        "Ahora puedes crear tu propio camino desde cero:<br><br>"
                        "1. Coloca 8 spawn points (azules)<br>"
                        "2. Coloca 1 checkpoint START (verde)<br>"
                        "3. Coloca checkpoints NORMAL (amarillos) - ¡los que necesites!<br>"
                        "4. Coloca 1 checkpoint FINISH (rojo)<br><br>"
                        " Usa 'Ver Ruta Actual' para ver el camino existente como referencia.")
                .arg(currentCity).arg(currentRoute));

        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error",
                QString("Error al cargar: %1").arg(e.what()));
        }
    }

    void viewExistingRoute() {
        if (currentCity.isEmpty()) {
            QMessageBox::warning(this, "Advertencia",
                "Primero carga una ruta con 'Cargar Ruta'");
            return;
        }

        QString cityFolder = getCityFolderName(currentCity);
        QString yamlPath = QString("server_src/city_maps/%1/%2.yaml")
            .arg(cityFolder).arg(currentRoute);

        try {
            YAML::Node config = YAML::LoadFile(yamlPath.toStdString());

            // Cargar spawn points EXISTENTES
            std::vector<SpawnPoint> existingSpawns;
            if (config["spawn_points"]) {
                for (const auto& sp : config["spawn_points"]) {
                    SpawnPoint point;
                    point.x = sp["x"].as<double>();
                    point.y = sp["y"].as<double>();
                    point.angle = sp["angle"].as<double>();
                    existingSpawns.push_back(point);
                }
            }

            // Cargar checkpoints EXISTENTES
            std::vector<Checkpoint> existingCheckpoints;
            if (config["checkpoints"]) {
                for (const auto& cp : config["checkpoints"]) {
                    Checkpoint checkpoint;
                    checkpoint.id = cp["id"].as<int>();
                    checkpoint.type = cp["type"].as<std::string>();
                    checkpoint.x = cp["x"].as<double>();
                    checkpoint.y = cp["y"].as<double>();
                    checkpoint.width = cp["width"].as<double>();
                    checkpoint.height = cp["height"].as<double>();
                    checkpoint.angle = cp["angle"].as<double>();
                    existingCheckpoints.push_back(checkpoint);
                }
            }

            // Mostrar la ruta EXISTENTE (solo para referencia, NO se guardan)
            mapView->displayCheckpoints(existingCheckpoints);
            mapView->displaySpawnPoints(existingSpawns);

            QMessageBox::information(this, "Ruta Existente Mostrada",
                QString("Se muestra la ruta actual como REFERENCIA:<br><br>"
                        "• Spawn Points: %1<br>"
                        "• Checkpoints: %2<br><br>"
                        "Estos NO se guardarán. Usa 'Limpiar Todo' y crea tu nuevo camino.")
                .arg(existingSpawns.size()).arg(existingCheckpoints.size()));

        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error",
                QString("Error al cargar ruta existente: %1").arg(e.what()));
        }
    }

    void saveRoute() {
        if (currentCity.isEmpty()) {
            QMessageBox::warning(this, "Advertencia", "Primero carga una ruta");
            return;
        }

        QString cityFolder = getCityFolderName(currentCity);
        QString yamlPath = QString("server_src/city_maps/%1/%2.yaml")
            .arg(cityFolder).arg(currentRoute);

        try {
            std::ofstream fout(yamlPath.toStdString());

            fout << "race:\n";
            fout << "  name: \"" << currentRoute.toStdString() << "\"\n";
            fout << "  city: \"" << currentCity.toStdString() << "\"\n";
            fout << "  description: \"" << raceDescription.toStdString() << "\"\n";
            fout << "  max_time_seconds: " << maxTimeSeconds << "\n";
            fout << "  total_laps: " << totalLaps << "\n\n";

            fout << "spawn_points:\n";
            for (const auto& sp : spawnPoints) {
                fout << "  - { x: " << sp.x << ", y: " << sp.y
                     << ", angle: " << sp.angle << " }\n";
            }
            fout << "\n";

            fout << "checkpoints:\n";
            for (const auto& cp : checkpoints) {
                fout << "  - { id: " << cp.id << ", type: \"" << cp.type
                     << "\", x: " << cp.x << ", y: " << cp.y
                     << ", width: " << cp.width << ", height: " << cp.height
                     << ", angle: " << cp.angle << " }\n";
            }

            fout.close();

            QMessageBox::information(this, "Éxito",
                "Ruta guardada correctamente!\n\nRecuerda reiniciar el servidor para aplicar los cambios.");

        } catch (const std::exception& e) {
            QMessageBox::critical(this, "Error",
                QString("Error al guardar: %1").arg(e.what()));
        }
    }

    void onCheckpointPlaced(double x, double y, QString type) {
        Checkpoint cp;
        cp.id = static_cast<int>(checkpoints.size());
        cp.type = type.toStdString();
        cp.x = x;
        cp.y = y;
        cp.angle = 0.0; // Siempre 0 como solicitaste

        // Dimensiones según el tipo
        if (type == "start" || type == "finish") {
            cp.width = 60.0;
            cp.height = 25.0;
        } else {
            cp.width = 50.0;
            cp.height = 20.0;
        }

        checkpoints.push_back(cp);
        mapView->displayCheckpoints(checkpoints);
        mapView->setNavigationMode();
        updateInfo();
    }

    void onSpawnPlaced(double x, double y) {
        SpawnPoint sp;
        sp.x = x;
        sp.y = y;
        sp.angle = 0.0;
        spawnPoints.push_back(sp);
        mapView->displaySpawnPoints(spawnPoints);
        mapView->setNavigationMode();
        updateInfo();
    }

    void deleteLastCheckpoint() {
        if (!checkpoints.empty()) {
            checkpoints.pop_back();
            // Renumerar IDs
            for (size_t i = 0; i < checkpoints.size(); ++i) {
                checkpoints[i].id = static_cast<int>(i);
            }
            mapView->displayCheckpoints(checkpoints);
            updateInfo();
        }
    }

    void deleteLastSpawn() {
        if (!spawnPoints.empty()) {
            spawnPoints.pop_back();
            mapView->displaySpawnPoints(spawnPoints);
            updateInfo();
        }
    }

    void clearAll() {
        auto reply = QMessageBox::question(this, "Confirmar",
            "¿Eliminar todos los checkpoints y spawn points?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            checkpoints.clear();
            spawnPoints.clear();
            mapView->clearMarkers();
            updateInfo();
        }
    }

    void updateInfo() {
        int startCount = 0, finishCount = 0, normalCount = 0;
        for (const auto& cp : checkpoints) {
            if (cp.type == "start") startCount++;
            else if (cp.type == "finish") finishCount++;
            else normalCount++;
        }

        QString info = QString(
            "<b>%1 - %2</b><br><br>"
            "<b>Checkpoints:</b> %3 total<br>"
            "• START: %4<br>"
            "• NORMAL: %5<br>"
            "• FINISH: %6<br><br>"
            "<b>Spawn Points:</b> %7<br><br>"
            "%8"
        ).arg(currentCity, currentRoute)
         .arg(checkpoints.size())
         .arg(startCount)
         .arg(normalCount)
         .arg(finishCount)
         .arg(spawnPoints.size())
         .arg((startCount == 1 && finishCount == 1 && spawnPoints.size() >= 8)
              ? "<span style='color:green'>Listo para guardar</span>"
              : "<span style='color:orange'>Necesitas 1 START, 1 FINISH y 8 spawns</span>");

        infoLabel->setText(info);
    }

public:
    explicit MapEditorWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
        setupUI();
    }
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    MapEditorWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"

