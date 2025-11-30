#ifndef CREATE_MATCH_WINDOW_H
#define CREATE_MATCH_WINDOW_H

#include <SDL_mixer.h>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QWidget>
#include <QProgressBar> // Necesario para el puntero
#include <vector>

#include "base_lobby.h"
#include "common_types.h"

struct CityInfo {
    QString name;
    QString imagePath;
    std::vector<QString> trackNames;
    std::vector<QString> trackImagePaths;
};

class CreateMatchWindow : public BaseLobby {
    Q_OBJECT

public:
    explicit CreateMatchWindow(QWidget* parent = nullptr);
    ~CreateMatchWindow();

    
    void showLoading(const QString& message, int progress);

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void matchCreated(const QString& matchName, int maxPlayers,
                      const std::vector<RaceConfig>& races);
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
    
    
    void setupLoadingUI();
    
    void loadCities();
    void updateCityDisplay();
    void updateTrackDisplay();
    void updateRaceList();
    void validateStep1();

    QPixmap backgroundImage;

    // Datos
    std::vector<CityInfo> cities;
    std::vector<RaceConfig> configuredRaces;
    size_t currentCityIndex;
    int currentTrackIndex;
    int currentEditingSlot;
    int totalRaces;

    // Paso 1 - Configuración básica
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

    // Paso 2 - Lista de carreras
    QWidget* step2Widget;
    QLabel* step2TitleLabel;
    QLabel* instructionLabel;
    QListWidget* raceListWidget;
    QLabel* progressLabel;
    QPushButton* confirmButton;
    QPushButton* backButtonStep2;

    // Paso 3 - Selector de ciudad/recorrido
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

    
    QWidget* loadingWidget;
    QLabel* loadingLabel;
    QProgressBar* loadingBar; // Barra de progreso
};

#endif  // CREATE_MATCH_WINDOW_H