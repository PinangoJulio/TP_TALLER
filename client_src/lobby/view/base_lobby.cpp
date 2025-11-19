#include "base_lobby.h"
#include <QFontDatabase>
#include <QIcon>

 BaseLobby:: BaseLobby(QWidget *parent)
    : QWidget(parent), musicToggleButton(nullptr), customFontId(-1) {
    
    musicOnIcon.addFile("assets/img/lobby/icons/music_on.png");
    musicOffIcon.addFile("assets/img/lobby/icons/music_off.png");
}

void  BaseLobby::setupMusicControl() {
    
    musicToggleButton = new QPushButton(this);

    musicToggleButton->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(255, 255, 255, 255);"
        "   border: 1px solid white;"
        "   border-radius: 5px;"
        "}"
        "QPushButton:hover {"
        "   border: 1px solid #00FF00;"
        "}"
    );
    
    musicToggleButton->setGeometry(645, 20, 35, 35);
    musicToggleButton->setCursor(Qt::PointingHandCursor);
    
    updateMusicButtonIcon();
    
    connect(musicToggleButton, &QPushButton::clicked, 
            this, & BaseLobby::onMusicToggleClicked);
}

void  BaseLobby::onMusicToggleClicked() {

    if (Mix_PausedMusic() == 1) {
        Mix_ResumeMusic();
    } else {
        Mix_PauseMusic();
    }
    updateMusicButtonIcon();
}

void  BaseLobby::updateMusicButtonIcon() {
    if (Mix_PausedMusic() == 1) {
        musicToggleButton->setIcon(musicOffIcon);
    } else {
        musicToggleButton->setIcon(musicOnIcon);
    }
    musicToggleButton->setIconSize(QSize(25, 25));
}