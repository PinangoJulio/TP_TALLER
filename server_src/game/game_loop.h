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

#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/queue.h"
#include "../../common_src/thread.h"
#include "../network/client_monitor.h"
#include "car.h"
#include "player.h"

// ✅ NUEVO: Incluir CollisionManager
#include "../../common_src/collision_manager.h"

#define NITRO_DURATION 12
#define SLEEP          16

class Race;

class GameLoop : public Thread {
private:
    // ---- ESTADO ----
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;
    std::atomic<bool> is_game_started;

    // ---- COMUNICACIÓN ----
    Queue<ComandMatchDTO>& comandos;
    ClientMonitor& queues_players;

    // ---- JUGADORES Y AUTOS ----
    std::map<int, std::unique_ptr<Player>> players;
    
    // ✅ NUEVO: Sistema de colisiones del mapa
    std::unique_ptr<CollisionManager> collision_manager;
    
    // ✅ NUEVO: Dimensiones del mapa actual
    int current_map_width;
    int current_map_height;

    bool all_players_disconnected() const;

    // ---- CARRERAS (RONDAS) ----
    std::vector<std::unique_ptr<Race>> races;
    size_t current_race_index;
    std::atomic<bool> current_race_finished;

    // ---- CONFIGURACIÓN CARRERA ACTUAL ----
    std::string current_map_yaml;
    std::string current_city_name;
    std::vector<std::tuple<float, float, float>> spawn_points;
    bool spawns_loaded;

    struct Checkpoint {
        int id;
        std::string type;
        float x, y;
        float width, height;
        float angle;
    };
    std::vector<Checkpoint> checkpoints;
    std::map<int, int> player_next_checkpoint;
    std::map<int, std::pair<float,float>> player_prev_pos;

    float checkpoint_tol_base = 1.5f;
    float checkpoint_tol_finish = 3.0f;
    int checkpoint_lookahead = 3;
    bool checkpoint_debug_enabled = true;

    // ---- TIEMPOS Y RESULTADOS ----
    std::chrono::steady_clock::time_point race_start_time;
    std::vector<std::map<int, uint32_t>> race_finish_times;
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
    
    // ✅ NUEVO: Método para validar colisiones
    void detectar_colisiones();
    bool is_position_valid(float x, float y, int player_level);
    bool can_move_to(float from_x, float from_y, float to_x, float to_y, int& player_level);
    
    void actualizar_estado_carrera();
    void verificar_ganadores();
    void enviar_estado_a_jugadores();

    GameState create_snapshot();

    void mark_player_finished(int player_id);
    void print_current_race_table() const;
    void print_total_standings() const;
    
    // ✅ NUEVO: Cargar collision manager para la carrera actual
    void load_collision_manager_for_current_race();

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