#include "../common_src/config.h" 
#include <box2d/box2d.h>                      
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

<<<<<<< HEAD
#include "../common_src/queue.h"
#include "../common_src/dtos.h"
#include "network/monitor.h"
#include "game/game_loop.h"

#define QUIT 'q'
=======
#include "server.h"

#define ERROR 1
#define SUCCESS 0
#define TIPO_AUTO_PRUEBA "DEPORTIVO"
>>>>>>> origin/main

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Error: Arguments are missing. Correct use : ./server <ruta_a_configuracion.yaml>" << std::endl;
        return ERROR;
    }

<<<<<<< HEAD
    // El archivo de configuración es el primer argumento (argv[1])
    const std::string ruta_config = argv[1];
    
    try {
        // 1. Cargar Configuración YAML
        Configuracion config(ruta_config);
        std::cout << "Servidor: Configuración YAML cargada correctamente." << std::endl;

        // 2. Crear infraestructura del servidor
        Monitor monitor;
        Queue<struct Command> game_queue;

        // 3. Crear y lanzar el GameSimulator con Box2D
        std::cout << "Servidor: Iniciando GameSimulator con Box2D..." << std::endl;
        GameSimulator game(monitor, game_queue, config);
        game.start();

        std::cout << "\n========================================" << std::endl;
        std::cout << "Servidor corriendo. Presiona 'q' + Enter para salir." << std::endl;
        std::cout << "========================================\n" << std::endl;

        // 4. Esperar comando de salida
        while (std::cin.get() != QUIT) {}

        // 5. Apagar el servidor limpiamente
        std::cout << "\nApagando servidor..." << std::endl;
        game.stop();
        game.join();
        
        std::cout << "Servidor: Simulacion finalizada correctamente." << std::endl;
=======
    try {
        const char *path_config = argv[1];
        Configuration::load_path(path_config);
        Server server((Configuration::get<std::string>("port")).c_str());
        server.start();
>>>>>>> origin/main

    } catch (const std::exception& e) {
        std::cerr << "Error initializing the Server :( " << e.what() << std::endl;
        return ERROR;
    }
<<<<<<< HEAD
    
    return 0;
=======
    return SUCCESS;
>>>>>>> origin/main
}