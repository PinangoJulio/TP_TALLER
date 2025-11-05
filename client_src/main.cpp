#include <iostream>
#include <exception>

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>

#include <QtWidgets/QApplication>
#include "lobby/lobby_window.h"  // ✅ CORREGIDO: Agregar lobby/
#include <iostream>

int main(int argc, char *argv[]) {
    try {
        // Inicializar la aplicación Qt
        QApplication app(argc, argv);

        // Crear y mostrar la ventana del Lobby
        LobbyWindow lobby;
        lobby.show();

        return app.exec();

    } catch (std::exception& e) {
        std::cerr << "Fallo fatal del Cliente: " << e.what() << std::endl;
        return 1;
    }
}