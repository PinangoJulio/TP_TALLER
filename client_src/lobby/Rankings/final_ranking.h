#ifndef FINAL_RANKING_WINDOW_H
#define FINAL_RANKING_WINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <vector>
#include <QString>
// Quitamos dependencia de common_types si no es necesaria, o la definimos aquí para ser autocontenidos
#include "../view/base_lobby.h"

// Definimos la estructura exacta que la vista espera recibir del cliente
struct PlayerResult {
    int rank;
    QString playerName;
    QString carName;
    QString totalTime; // Tiempo ya formateado como string (ej: "02:15.400")
};

class FinalRankingWindow : public BaseLobby {
    Q_OBJECT

public:
    explicit FinalRankingWindow(QWidget *parent = nullptr);
    ~FinalRankingWindow();

    void setResults(const std::vector<PlayerResult>& results);

signals:
    void returnToLobbyRequested();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    QWidget* createRankingCard(const PlayerResult& result); 

    QPixmap backgroundImage;
    QLabel* titleLabel;
    
    QWidget* resultsContainer; 
    QVBoxLayout* resultsLayout;
    QScrollArea* scrollArea;

    QPushButton* backButton;
    int customFontId;
};

#endif // FINAL_RANKING_WINDOW_H