#include <QMessageBox>
#include <QtWidgets/QApplication>
#include <exception>
#include <iostream>

#include "client.h"
#include "lobby/controller/lobby_controller.h"

int main(int argc, char* argv[]) {
    try {
        // Inicializar Qt
        QApplication app(argc, argv);

        // Configuración del servidor (por ahora hardcodeado)
        // TODO: Leer de argumentos de línea de comandos
        QString host = "localhost";
        QString port = "8080";

        std::cout << "=== Need for Speed 2D - Cliente ===" << std::endl;
        std::cout << "Conectando a " << host.toStdString() << ":" << port.toStdString() << std::endl;

        Client client(host.toStdString().c_str(), port.toStdString().c_str());
        client.start();

        // NO llamar a app.exec() aquí porque client.start() ya maneja todo el ciclo:
        // 1. Lobby Qt (con su propio QEventLoop)
        // 2. SDL game loop
        // El programa termina cuando termina el juego SDL

        std::cout << "=== Cliente finalizado ===" << std::endl;
        return 0;

    } catch (std::exception& e) {
        std::cerr << "  Fallo fatal del Cliente: " << e.what() << std::endl;

        QMessageBox::critical(nullptr, "Error Fatal",
                              QString("No se pudo iniciar el cliente:\n%1").arg(e.what()));

        return 1;
    }
}
