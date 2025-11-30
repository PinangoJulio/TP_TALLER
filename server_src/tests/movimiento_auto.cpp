#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../game/game_loop.h"
#include "../network/client_monitor.h"

// Mock o Stub simple si no quieres instanciar todo el monitor real
// Pero como ya linkeamos todo, usamos el real.

int main(int argc, char const* argv[]) {
    try {
        // Definimos rutas de prueba (Asegúrate de que estos archivos existan)
        std::string mapa_yaml = "server_src/city_maps/Vice City/Vice City.yaml";
        std::string carrera_yaml = "server_src/city_maps/Vice City/race_1.yaml";

        // Si el usuario pasa argumentos, los usamos
        if (argc > 1) mapa_yaml = argv[1];
        if (argc > 2) carrera_yaml = argv[2];

        // Colas y Monitor
        Queue<ComandMatchDTO> comandos;
        ClientMonitor monitor; // Monitor vacío (no enviará nada real a nadie)

        std::cout << "Iniciando GameLoop de prueba..." << std::endl;
        std::cout << "Mapa: " << mapa_yaml << std::endl;
        std::cout << "Carrera: " << carrera_yaml << std::endl;

        // CORRECCIÓN: Constructor con 4 argumentos
        GameLoop loop(comandos, monitor, mapa_yaml, carrera_yaml);

        // Iniciamos el loop
        loop.start();

        // Simulamos un poco de tiempo (ej: 5 segundos)
        std::cout << "Simulando 5 segundos..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));

        // Detenemos
        loop.stop();
        loop.join();
        
        std::cout << "Test finalizado con éxito." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Excepción en test: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}