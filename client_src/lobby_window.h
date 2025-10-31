#ifndef LOBBY_WINDOW_H
#define LOBBY_WINDOW_H

#pragma once
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>

// La clase principal del Lobby hereda de QWidget para ser una ventana/formulario
class LobbyWindow : public QWidget {
    Q_OBJECT // Macró obligatorio para usar signals y slots de Qt

private:
    QLabel* titleLabel;
    QLabel* statusLabel;
    QPushButton* connectButton;
    QPushButton* quitButton;

public:
    LobbyWindow(QWidget *parent = nullptr);
    ~LobbyWindow() override;

private slots:
    // Slot que se conectará al botón de Conectar
    void onConnectClicked();
};

#endif //LOBBY_WINDOW_H