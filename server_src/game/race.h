#ifndef RACE_H
#define RACE_H

#include <atomic>
#include <memory>
#include <string>
#include <iostream>
#include "game_loop.h"
#include "../../common_src/queue.h"
#include "../network/client_monitor.h"

class Race {
private:
    std::unique_ptr<GameLoop> gameLoop;
    Queue<ComandMatchDTO>& commandQueue;
    ClientMonitor& broadcaster;
    std::string city_name;
    std::string map_yaml_path;

public:
    Race(Queue<ComandMatchDTO>& cmdQueue,
         ClientMonitor& brdcstr,
         const std::string& city,
         const std::string& yaml_mapa)
        : commandQueue(cmdQueue),
          broadcaster(brdcstr),
          city_name(city),
          map_yaml_path(yaml_mapa)
    {
        gameLoop = std::make_unique<GameLoop>(commandQueue, broadcaster, map_yaml_path);
    }

    void start() {
        std::cout << "[Race] Iniciando carrera en " << city_name
                  << " (mapa: " << map_yaml_path << ")\n";
        //gameLoop->start();  // Descomentarás esto cuando quieras activar el GameLoop
    }

    void stop() {
        std::cout << "[Race] Deteniendo carrera en " << city_name << "\n";
        gameLoop->stop_race();
        gameLoop->join();
    }

    // ✅ Agregar jugadores al GameLoop
    void add_player_to_gameloop(int player_id, const std::string& name,
                                const std::string& car_name, const std::string& car_type) {
        gameLoop->add_player(player_id, name, car_name, car_type);
    }

    // ✅ Actualizar estado de jugadores
    void set_player_ready(int player_id, bool ready) {
        gameLoop->set_player_ready(player_id, ready);
    }

    // ✅ Configurar parámetros de la carrera
    void set_total_laps(int laps) { gameLoop->set_total_laps(laps); }
    void set_city(const std::string& city) { gameLoop->set_city_name(city); }

    const std::string& get_city_name() const { return city_name; }
    const std::string& get_map_path() const { return map_yaml_path; }

    /* Cuando la carrera termina, detener y limpiar el GameLoop en el destructor de Race,
     * para evitar hilos colgando*/
    ~Race() {
        if (gameLoop && gameLoop->is_alive()) { // <-- Asegurarse de que solo se hace stop/join si es necesario
            gameLoop->stop();
            gameLoop->join();
        }
    }
};

#endif // RACE_H
