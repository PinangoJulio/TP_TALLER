#ifndef NAME_INPUT_WINDOW_H
#define NAME_INPUT_WINDOW_H

#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QWidget>

#include "base_lobby.h"

class NameInputWindow : public BaseLobby {
    Q_OBJECT

public:
    explicit NameInputWindow(QWidget* parent = nullptr);
    ~NameInputWindow();

    QString getPlayerName() const;

protected:
    void paintEvent(QPaintEvent* event) override;

signals:
    void nameConfirmed(const QString& playerName);

private slots:
    void onConfirmClicked();
    void onTextChanged(const QString& text);

private:
    void setupUI();
    bool validateName(const QString& name);

    QPixmap backgroundImage;

    QLabel* titleLabel;
    QLabel* instructionLabel;
    QLabel* errorLabel;
    QLineEdit* nameInput;
    QPushButton* confirmButton;

    QString playerName;
};

#endif  // NAME_INPUT_WINDOW_H
