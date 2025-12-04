#ifndef FINAL_RANKING_WINDOW_H
#define FINAL_RANKING_WINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <vector>
#include <QString>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <QLabel>

#include "../view/base_lobby.h"


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