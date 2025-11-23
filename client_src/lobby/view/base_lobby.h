#ifndef BASE_LOBBY_H
#define BASE_LOBBY_H

#include <SDL_mixer.h>

#include <QIcon>
#include <QPushButton>
#include <QWidget>

class BaseLobby : public QWidget {
    Q_OBJECT

public:
    explicit BaseLobby(QWidget* parent = nullptr);
    virtual ~BaseLobby() = default;

protected:
    QPushButton* musicToggleButton;
    int customFontId;

    void setupMusicControl();

private slots:
    void onMusicToggleClicked();

private:
    QIcon musicOnIcon;
    QIcon musicOffIcon;
    void updateMusicButtonIcon();
};

#endif  // BASE_LOBBY_H
