#ifndef MATCH_SELECTION_WINDOW_H
#define MATCH_SELECTION_WINDOW_H

#include <QLabel>
#include <QWidget>
#include <QPixmap>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>


#include "common_src/lobby_protocol.h"  
#include "base_lobby.h"  


class MatchSelectionWindow : public  BaseLobby{
    Q_OBJECT

public:
    explicit MatchSelectionWindow(QWidget *parent = nullptr);
    void updateGamesList(const std::vector<GameInfo>& games);
    QListWidgetItem* getSelectedItem() const;
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