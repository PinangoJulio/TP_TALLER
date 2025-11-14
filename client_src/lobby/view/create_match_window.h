#ifndef CREATE_MATCH_WINDOW_H
#define CREATE_MATCH_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include <QStackedWidget>
#include <QPixmap>
#include <vector>
#include "common_types.h"
#include <QComboBox>          // <--- 1. AGREGAR ESTE INCLUDE
#include <SDL2/SDL_mixer.h>
struct CityInfo {
    QString name;
    QString imagePath;
    std::vector<QString> trackNames;
    std::vector<QString> trackImagePaths;
};

class CreateMatchWindow : public QWidget {
    Q_OBJECT

public:
    explicit CreateMatchWindow(QWidget *parent = nullptr);
    ~CreateMatchWindow();

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void matchCreated(const QString& matchName, int maxPlayers, const std::vector<RaceConfig>& races);
    void backRequested();

private slots:
    // Paso 1: Configuración básica
    void onMatchNameChanged(const QString& text);
    void onNextToRaceList();
    
    // Paso 2: Lista de carreras
    void onRaceSlotClicked(QListWidgetItem* item);
    void onConfirmRaceList();
    void onBackFromRaceList();
    
    // Paso 3: Selector de ciudad/recorrido
    void onPreviousCity();
    void onNextCity();
    void onCitySelected();
    void onPreviousTrack();
    void onNextTrack();
    void onConfirmSelection();
    void onBackFromSelector();

private:
    void setupStep1UI();
    void setupStep2UI();
    void setupStep3UI();
    void loadCities();
    void updateCityDisplay();
    void updateTrackDisplay();
    void updateRaceList();
    void validateStep1();
    
    QPixmap backgroundImage;
    int customFontId;
    
    // Datos
    std::vector<CityInfo> cities;
    std::vector<RaceConfig> configuredRaces;
    size_t currentCityIndex;
    int currentTrackIndex;
    int currentEditingSlot; // Índice q se está editando
    int totalRaces;
    
    //Paso 1 - Configuración básica
    QStackedWidget* stepsStack;
    QWidget* step1Widget;
    QLabel* step1TitleLabel;
    QLineEdit* matchNameInput;
    QLabel* matchNameLabel;
    QSpinBox* maxPlayersSpinBox;
    QLabel* maxPlayersLabel;
    QSpinBox* numRacesSpinBox;
    QLabel* numRacesLabel;
    QPushButton* nextStepButton;
    QPushButton* backButtonStep1;
    
    //Paso 2 - Lista de carreras
    QWidget* step2Widget;
    QLabel* step2TitleLabel;
    QLabel* instructionLabel;
    QListWidget* raceListWidget;
    QLabel* progressLabel;
    QPushButton* confirmButton;
    QPushButton* backButtonStep2;
    
    //Paso 3 - Selector de ciudad/recorrido
    QWidget* step3Widget;
    QLabel* step3TitleLabel;
    QLabel* editingLabel;
    
    QLabel* citySectionLabel;
    QLabel* cityNameLabel;
    QLabel* cityImageLabel;
    QPushButton* prevCityButton;
    QPushButton* nextCityButton;
    
    QLabel* trackSectionLabel;
    QLabel* trackNameLabel;
    QLabel* trackImageLabel;
    QPushButton* prevTrackButton;
    QPushButton* nextTrackButton;
    
    QPushButton* confirmSelectionButton;
    QPushButton* backButtonStep3;
};

#endif // CREATE_MATCH_WINDOW_H