#ifndef FINAL_RANKING_WINDOW_H
#define FINAL_RANKING_WINDOW_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <vector>
#include "../lobby/view/base_lobby.h"
#include "../lobby/view/common_types.h"

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
    
    // Contenedor para la lista de tarjetas
    QWidget* resultsContainer; 
    QVBoxLayout* resultsLayout;
    QScrollArea* scrollArea;

    QPushButton* backButton;
    int customFontId;
};

#endif // FINAL_RANKING_WINDOW_H