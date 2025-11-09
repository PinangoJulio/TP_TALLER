#include <iostream>
#include <exception>
#include <QtWidgets/QApplication>
#include <SDL2/SDL.h>
#include "lobby/front/lobby_window.h"

int main(int argc, char *argv[]) {
    try {
        // Inicializar Qt
        QApplication app(argc, argv);
        
        // Crear y mostrar lobby principal (sin nombre todavía)
        LobbyWindow* lobby = new LobbyWindow();
        lobby->show();
        
        // Bucle de eventos Qt
        return app.exec();
        
    } catch (std::exception& e) {
        std::cerr << "Fallo fatal del Cliente: " << e.what() << std::endl;
        return 1;
    }
}