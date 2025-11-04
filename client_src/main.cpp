#include <iostream>
#include <exception>

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>

#include <QtWidgets/QApplication>
#include "lobby_window.h"  // Está en client_src/lobby/, CMake sabe dónde buscar

int main(int argc, char *argv[]) {
    try {
        // Inicializar la aplicación Qt
        QApplication app(argc, argv);

        // Crear y mostrar la ventana del Lobby
        LobbyWindow lobby;
        lobby.show(); // Mostrar la ventana

        // Iniciar el bucle de eventos de Qt. Bloquea hasta que se cierra la ventana.
        return app.exec();

    } catch (std::exception& e) {
        std::cerr << "Fallo fatal del Cliente: " << e.what() << std::endl;
        return 1;
    }
}s