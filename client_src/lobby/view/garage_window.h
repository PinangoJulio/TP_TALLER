#ifndef GARAGE_WINDOW_H
#define GARAGE_WINDOW_H

#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>
#include <cstdint>
#include <vector>

#include "base_lobby.h"

struct CarInfo {
    QString name;
    QString imagePath;
    QString type;  // tipo de auto (sport, truck, classic)
    int speed;
    int acceleration;
    int handling;
    int durability;
};

class GarageWindow : public BaseLobby {
    Q_OBJECT

public:
    explicit GarageWindow(QWidget* parent = nullptr);
    ~GarageWindow();

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void carSelected(const CarInfo& car);
    void backRequested();

private slots:
    void onPreviousCar();
    void onNextCar();
    void onSelectCar();
    void onBackClicked();

private:
    void setupUI();
    void loadCars();
    void loadDefaultCars();  // ✅ Método de fallback para cargar autos hardcodeados
    void updateCarDisplay();
    void createStatLabels();
    void drawStatBar(QPainter& painter, int x, int y, int width, int value, const QString& label);

    QPixmap backgroundImage;

    std::vector<CarInfo> cars;
    size_t currentCarIndex;

    QLabel* titleLabel;
    QLabel* carNameLabel;
    QLabel* carImageLabel;
    QWidget* statsPanel;

    // Widgets estadísticas
    std::vector<QLabel*> statNameLabels;
    std::vector<QWidget*> statBarBackgrounds;
    std::vector<QWidget*> statBarFills;
    std::vector<QLabel*> statValueLabels;

    QPushButton* prevButton;
    QPushButton* nextButton;
    QPushButton* selectButton;
    QPushButton* backButton;
};

#endif  // GARAGE_WINDOW_H
