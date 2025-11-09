#ifndef MATCH_SELECTION_WINDOW_H
#define MATCH_SELECTION_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QPixmap>


#include "common_src/lobby_protocol.h"  // Para GameInfo


class MatchSelectionWindow : public QWidget {
    Q_OBJECT

public:
    explicit MatchSelectionWindow(QWidget *parent = nullptr);
    void updateGamesList(const std::vector<GameInfo>& games);
    ~MatchSelectionWindow();

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

signals:
    void joinMatchRequested(const QString& matchId);
    void createMatchRequested();
    void refreshRequested();
    void backToLobby();

private slots:
    void onJoinMatchClicked();
    void onCreateMatchClicked();
    void onBackClicked();
    void onMatchSelected(QListWidgetItem* item);

private:
    void setupUI();
   // void loadPlaceholderMatches();
    
    QPixmap backgroundImage;
    
    QLabel* titleLabel;
    QListWidget* matchList;
    QPushButton* joinButton;
    QPushButton* createButton;
    QPushButton* backButton;
    QPushButton* refreshButton;
    
    int customFontId;
    QString selectedMatchId;
};

#endif // MATCH_SELECTION_WINDOW_H