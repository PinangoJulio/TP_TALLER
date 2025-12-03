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
#include <tuple>
#include <box2d/box2d.h>

#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/queue.h"
#include "../../common_src/thread.h"
#include "../network/client_monitor.h"
#include "car.h"
#include "player.h"
#include "physics_constants.h"

#define NITRO_DURATION 12
#define SLEEP          16 // Ticks de 16ms (~60 updates por segundo para movimiento fluido)

// Forward declaration
class Race;

/*
 * GameLoop - Servidor de juego multijugador
 * ==========================================
 *
 * Arquitectura basada en el patrón estándar de game servers:
 *
 *   while (not exit) {
 *     1. receive_updates_from_clients();     // Leer comandos (ACCELERATE, TURN, etc)
 *     2. update_game_logic_and_physics();    // Actualizar posiciones, colisiones, checkpoints
 *     3. broadcast_updates_to_clients();     // Enviar GameState a todos
 *     4. sleep_and_calc_next_iteration(60);  // Mantener 60 FPS constantes
 *   }
 *
 * Responsabilidades:
 * ------------------
 *   Gestiona TODAS las carreras de una partida como "rondas"
 *   Recibe comandos de jugadores (acelerar, frenar, girar, nitro)
 *   Actualiza física de los autos (velocidad, dirección, fricción)
 *   Detecta colisiones (contra paredes, otros autos, obstáculos del YAML)
 *   Actualiza estado de juego (checkpoints, tiempos, posiciones)
 *   Envía el estado completo a los clientes vía broadcast (ClientMonitor)
 *   Mantiene 60 FPS constantes para movimiento fluido
 *
 * Flujo de una partida:
 * ---------------------
 * 1. Constructor: GameLoop() - Se crea el thread (pero no inicia)
 * 2. Match llama a add_race() para configurar las carreras
 * 3. Match llama a add_player() por cada jugador
 * 4. Match llama a start_game() cuando todos están listos
 * 5. El thread comienza el loop: receive → update → broadcast → sleep
 * 6. Al terminar todas las carreras, imprime tabla de posiciones final
 *
 * Control de timing:
 * ------------------
 * - SLEEP = 16ms (~60 FPS)
 * - Usa std::this_thread::sleep_until() para evitar acumulación de lag
 * - Compensa automáticamente si una iteración se atrasa
 *
 * Sincronización con clientes:
 * ----------------------------
 * - Los clientes también deben correr a ~60 FPS (o múltiplo)
 * - El cliente interpola entre snapshots para suavizar movimiento
 * - Todos reciben el MISMO snapshot simultáneamente (broadcast)
 */

class GameLoop : public Thread {
private:
    // ---- ESTADO ----
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;  // Todas las carreras completadas
    
    
    std::atomic<bool> is_game_started;

    // ---- COMUNICACIÓN ----
    Queue<ComandMatchDTO>& comandos;  // Comandos de jugadores (ACCELERATE, BRAKE, etc)
    ClientMonitor& queues_players;    // Queues para broadcast a jugadores

    // ---- JUGADORES Y AUTOS ----
    std::map<int, std::unique_ptr<Player>> players;  // player_id → Player (contiene Car)
    bool all_players_disconnected() const;

    // ---- CARRERAS (RONDAS) ----
    std::vector<std::unique_ptr<Race>> races;  // Todas las carreras de la partida
    size_t current_race_index;                 // Carrera actual (0-based)
    std::atomic<bool> current_race_finished;   // Carrera actual terminada

    // ---- CONFIGURACIÓN CARRERA ACTUAL ----
    std::string current_map_yaml;
    std::string current_city_name;
    std::vector<std::tuple<float, float, float>> spawn_points;  // (x, y, angle)
    bool spawns_loaded;

    struct Checkpoint {
        int id;
        std::string type;  // "start", "normal", "finish"
        float x, y;
        float width, height;
        float angle;       // grados
    };
    std::vector<Checkpoint> checkpoints;
    std::map<int, int> player_next_checkpoint;           // player_id → índice del próximo cp esperado
    std::map<int, std::pair<float,float>> player_prev_pos; // posición previa por jugador

    float checkpoint_tol_base = 1.5f;
    float checkpoint_tol_finish = 3.0f;
    int checkpoint_lookahead = 3;
    bool checkpoint_debug_enabled = true; // habilita prints de debug

    // ---- TIEMPOS Y RESULTADOS ----
    std::chrono::steady_clock::time_point race_start_time;

    // Tiempos de finalización por carrera: race_index → (player_id → tiempo_ms)
    std::vector<std::map<int, uint32_t>> race_finish_times;

    // Tabla general acumulada: player_id → tiempo_total_ms
    std::map<int, uint32_t> total_times;

    // ---- MÉTODOS PRIVADOS ----
    void load_spawn_points_for_current_race();
    void reset_players_for_race();
    void start_current_race();
    void finish_current_race();
    bool all_players_finished_race() const;

    void load_checkpoints_for_current_race();
    bool check_player_crossed_checkpoint(int player_id, const Checkpoint& cp);
    void update_checkpoints();

    void procesar_comandos();
    void actualizar_fisica();
    void detectar_colisiones();
    void actualizar_estado_carrera();
    void verificar_ganadores();
    void enviar_estado_a_jugadores();

    GameState create_snapshot();

    void mark_player_finished(int player_id);
    void mark_player_finished_with_time(int player_id, uint32_t finish_time_ms);
    void print_current_race_table() const;
    void print_total_standings() const;

    b2WorldId physics_world_id;
    bool physics_world_created;
    const float TIME_STEP = 1.0f / 60.0f;
    const int32_t VELOCITY_ITERATIONS = 8;
    const int32_t POSITION_ITERATIONS = 3;

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);
  
    
    void start_game();

    // ---- CONFIGURACIÓN DE CARRERAS ----
    void add_race(const std::string& city, const std::string& yaml_path);
    void set_races(std::vector<std::unique_ptr<Race>> race_configs);

    // ---- AGREGAR JUGADORES ----
    void add_player(int player_id, const std::string& name, const std::string& car_name,
                    const std::string& car_type);
    void delete_player_from_match(int player_id);

    // ---- ESTADO DE JUGADORES ----
    void set_player_ready(int player_id, bool ready);

    // ---- CONTROL ----
    void run() override;
    void stop_match();
    bool is_alive() const override { return is_running.load(); }

    // ---- DEBUG ----
    void print_match_info() const;

    ~GameLoop() override;
};

#endif  // GAME_LOOP_H
