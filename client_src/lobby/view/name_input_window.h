#ifndef NAME_INPUT_WINDOW_H
#define NAME_INPUT_WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>

class NameInputWindow : public QWidget {
    Q_OBJECT

public:
    explicit NameInputWindow(QWidget *parent = nullptr);
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
    
    int customFontId;
    QString playerName;
};

#endif // NAME_INPUT_WINDOW_H