#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>

#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../game/game_loop.h"
#include "../network/client_monitor.h"
#include "../game/race.h"

int main(int argc, char const* argv[]) {
    try {
        // Rutas por defecto (ajustadas a tu estructura)
        std::string mapa_yaml = "server_src/city_maps/Vice City/vice-city.yaml";
        std::string carrera_yaml = "server_src/city_maps/Vice City/Autopista.yaml";

        if (argc > 1) mapa_yaml = argv[1];
        if (argc > 2) carrera_yaml = argv[2];

        Queue<ComandMatchDTO> comandos;
        ClientMonitor monitor; 

        std::cout << "Iniciando GameLoop de prueba..." << std::endl;
        std::cout << "Mapa Base: " << mapa_yaml << std::endl;
        std::cout << "Config Carrera: " << carrera_yaml << std::endl;

        // 1. Crear GameLoop
        GameLoop loop(comandos, monitor);

        // 2. Configurar Carrera
        std::vector<std::unique_ptr<Race>> races;
        auto race = std::make_unique<Race>("Vice City", mapa_yaml, carrera_yaml);
        races.push_back(std::move(race));
        loop.set_races(std::move(races));

        // 3. Agregar Jugador
        loop.add_player(1, "Tester", "Leyenda Urbana", "Classic");
        loop.set_player_ready(1, true);

        // 4. Iniciar Loop
        loop.start();

        // --- PRUEBA DE MOVIMIENTO ---
        std::cout << "\n>>> ENVIANDO COMANDO: ACELERAR <<<" << std::endl;
        
        // Enviar comando de aceleración
        ComandMatchDTO cmd;
        cmd.player_id = 1;
        cmd.command = GameCommand::ACCELERATE;
        
        // Mantener el acelerador presionado enviando comandos seguidos
        // (En un juego real el cliente manda esto constantemente o por eventos)
        for(int i = 0; i < 100; i++) {
             comandos.push(cmd);
             std::this_thread::sleep_for(std::chrono::milliseconds(16)); // Simular 16ms entre inputs
             
             if (i % 20 == 0) std::cout << "." << std::flush; // Feedback visual
        }
        std::cout << "\n";

        std::cout << "Simulando inercia restante..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));

        loop.stop();
        loop.join();
        
        std::cout << "Test finalizado con éxito." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Excepción en test: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}