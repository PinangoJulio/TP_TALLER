#include <iostream>
#include <exception>
#include <QtWidgets/QApplication>
#include <QMessageBox>
#include "controller/lobby_controller.h"

int main(int argc, char *argv[]) {
    // Inicializar Qt
    QApplication app(argc, argv);
    
    // Configuración del servidor (por ahora hardcodeado)
    // TODO: Leer de argumentos de línea de comandos
    QString host = "localhost";
    QString port = "8080";
    
    std::cout << "=== Need for Speed 2D - Cliente ===" << std::endl;
    std::cout << "Servidor configurado: " << host.toStdString() 
              << ":" << port.toStdString() << std::endl;
    
    // Crear controlador (NO se conecta todavía)
    LobbyController controller(host, port);
    
    // Iniciar el flujo (muestra lobby principal)
    controller.start();
    
    // Bucle de eventos Qt
    return app.exec();
}