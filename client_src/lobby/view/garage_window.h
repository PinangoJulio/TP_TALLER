#ifndef GARAGE_WINDOW_H
#define GARAGE_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <vector>

struct CarInfo {
    QString name;
    QString imagePath;
    QString type;  // <-- nuevo campo (ej. "sport", "truck", "classic")
    int speed;
    int acceleration;
    int handling;
    int durability;
};

class GarageWindow : public QWidget {
    Q_OBJECT

public:
    explicit GarageWindow(QWidget *parent = nullptr);
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
    
    // Widgets para las estadísticas
    std::vector<QLabel*> statNameLabels;
    std::vector<QWidget*> statBarBackgrounds;
    std::vector<QWidget*> statBarFills;
    std::vector<QLabel*> statValueLabels;
    
    QPushButton* prevButton;
    QPushButton* nextButton;
    QPushButton* selectButton;
    QPushButton* backButton;
    
    int customFontId;
};

#endif // GARAGE_WINDOW_H