#ifndef LOBBY_WINDOW_H
#define LOBBY_WINDOW_H

#pragma once
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QPixmap>
#include <SDL2/SDL_mixer.h>

class LobbyWindow : public QWidget {
    Q_OBJECT

private:
    QLabel* titleLabel;
    QLabel* statusLabel;
    QPushButton* connectButton;
    QPushButton* quitButton;
    
    // Imagen de fondo con Qt
    QPixmap backgroundImage;
    
    // SDL solo para audio
    Mix_Music* backgroundMusic;
    bool audioInitialized;
    
    // Fuente personalizada por el momento solo uso classic-arcade
    int customFontId;
    
    void initAudio();
    void cleanupAudio();
    void loadMusic(const char* musicPath);

protected:
    void paintEvent(QPaintEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

public:
    LobbyWindow(QWidget *parent = nullptr);
    ~LobbyWindow() override;

private slots:
    void onConnectClicked();
};

#endif //LOBBY_WINDOW_H