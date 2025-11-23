#include <QMessageBox>
#include <QtWidgets/QApplication>
#include <exception>
#include <iostream>

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

        // Crear controlador (se conecta al servidor)
        LobbyController controller(host, port);

        // Iniciar el flujo (muestra ventana de nombre)
        controller.start();

        // Bucle de eventos Qt
        return app.exec();

    } catch (std::exception& e) {
        std::cerr << "  Fallo fatal del Cliente: " << e.what() << std::endl;

        QMessageBox::critical(nullptr, "Error Fatal",
                              QString("No se pudo iniciar el cliente:\n%1").arg(e.what()));

        return 1;
    }
}
