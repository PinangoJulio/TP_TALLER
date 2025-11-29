#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/queue.h"
#include "../../common_src/thread.h"
#include "../network/client_monitor.h"
#include "../../common_src/collision_manager.h"
#include "car.h"
#include "player.h"

#define NITRO_DURATION 12
#define SLEEP          250

/*
 * GameLoop:
 * - Recibe Comandos de jugadores (acelerar, frenar, girar, usar nitro)
 * - Actualiza la física de los autos (Box2D)
 * - Detecta colisiones (contra paredes, otros autos, obstáculos YAML)
 * - Actualiza estado de juego (vueltas, checkpoints, tiempos)
 * - Envía el estado actualizado a los clientes via broadcast
 */

class GameLoop : public Thread {
private:
    // ---- ESTADO ----
    std::atomic<bool> is_running;
    std::atomic<bool> race_finished;

    // ---- COMUNICACIÓN ----
    Queue<ComandMatchDTO>& comandos;  // Comandos de jugadores (ACCELERATE, BRAKE, etc)
    ClientMonitor& queues_players;    // Queues para broadcast a jugadores

    // ---- JUGADORES Y AUTOS ----
    // Cada Player contiene su Car, no necesitamos un mapa separado
    // La clave (int) es el player_id (también client_id)
    std::map<int, std::unique_ptr<Player>> players;  // player_id → Player (contiene Car)

    // ---- MAPA Y CONFIGURACIÓN ----
    std::string yaml_path;  // Ruta al YAML del mapa
    std::string city_name;  // Nombre de la ciudad
    int total_laps;         // Vueltas totales de la carrera
    // Mapa mapa;

    // ---- TIEMPOS ----
    std::chrono::steady_clock::time_point race_start_time;

    // ---- Manager de colidiones----
    CollisionManager collision_manager;


    void reset_players_spawn_positions();  // Resetea posiciones de spawn (al iniciar carrera)
    void execute_player_movement(Player *player, const ComandMatchDTO& comando);
    void procesar_comandos();
    void actualizar_fisica();
    void detectar_colisiones();
    void actualizar_estado_carrera();
    bool check_crossed_finish_line(Player* player);
    bool check_match_ended();
    void enviar_estado_a_jugadores();
    void verificar_ganadores();

    // Crear snapshot del estado actual
    GameState create_snapshot();

    // Auxiliar
    std::string to_kebab_case(std::string str);
    
public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues, 
             const std::string& yaml_path, const std::string& city);
    // ---- AGREGAR JUGADORES ----
    void add_player(int player_id, const std::string& name, const std::string& car_name,
                    const std::string& car_type);
    void delete_player_from_match(const int player_id);

    // ---- ESTADO DE JUGADORES ----
    void set_player_ready(int player_id, bool ready);


    // ---- CONFIGURACIÓN ----
    void set_total_laps(int laps) { total_laps = laps; }
    void set_city_name(const std::string& city) { city_name = city; }

    // ---- CONTROL ----
    void run() override;
    void stop_race();
    bool is_alive() const override { return is_running.load(); }

    // ---- DEBUG ----
    void print_race_info() const;

    ~GameLoop() override;
};

#endif  // GAME_LOOP_H
