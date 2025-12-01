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
#include "car.h"
#include "player.h"

#define NITRO_DURATION 12
#define SLEEP          250

// Forward declaration
class Race;

/*
 * GameLoop:
 * - Gestiona TODAS las carreras de una partida como "rondas"
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
    std::atomic<bool> match_finished;  // Todas las carreras completadas
    
    // ✅ NUEVO: Señal para esperar la orden explícita del Match
    std::atomic<bool> start_game_signal; 

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

    void procesar_comandos();
    void actualizar_fisica();
    void detectar_colisiones();
    void actualizar_estado_carrera();
    void verificar_ganadores();
    void enviar_estado_a_jugadores();

    GameState create_snapshot();

    void mark_player_finished(int player_id);
    void print_current_race_table() const;
    void print_total_standings() const;

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);

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
    // ✅ NUEVO: Método para desbloquear el loop
    void start_game();
    
    void run() override;
    void stop_match();
    bool is_alive() const override { return is_running.load(); }

    // ---- DEBUG ----
    void print_match_info() const;

    ~GameLoop() override;
};

#endif  // GAME_LOOP_H
