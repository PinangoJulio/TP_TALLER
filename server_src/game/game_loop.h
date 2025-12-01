#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <map>
#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <chrono>
#include <box2d/box2d.h>

#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/thread.h" 
#include "../network/client_monitor.h"
#include "player.h"

// ---- MÓDULOS DE FÍSICA (BOX2D) ----
#include "map_loader.h"       
#include "collision_handler.h"
#include "checkpoint_manager.h"
#include "obstacle.h"

class Race;

#define TIME_STEP (1.0f / 60.0f)
#define SUB_STEPS 4
#define SLEEP 16

class GameLoop : public Thread { 
private:
    // ---- ESTADO DE CONTROL ----
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;
    std::atomic<bool> started_signal; 

    // ---- COMUNICACIÓN ----
    Queue<ComandMatchDTO>& comandos;
    ClientMonitor& queues_players;
    
    // ---- BOX2D WORLD & SISTEMAS ----
    b2WorldId worldId;
    ObstacleManager obstacleManager;
    MapLoader mapLoader;
    CollisionHandler collisionHandler;   
    CheckpointManager checkpointManager; 

    // ---- JUGADORES ----
    std::map<int, std::unique_ptr<Player>> players;

    // ---- GESTIÓN DE CARRERAS ----
    std::vector<std::unique_ptr<Race>> races;
    int current_race_index;
    std::atomic<bool> current_race_finished;
    
    // Configuración actual
    std::string current_map_yaml;
    std::string current_city_name;
    std::chrono::steady_clock::time_point race_start_time;
    
    // CORRECCIÓN: Variable que faltaba
    bool spawns_loaded;
    
    // Estadísticas
    std::vector<std::map<int, uint32_t>> race_finish_times; 
    std::map<int, uint32_t> total_times;

    // ---- MÉTODOS INTERNOS ----
    void procesar_comandos();
    void actualizar_fisica();
    void detectar_colisiones();
    void actualizar_logica_juego();
    void enviar_estado();
    
    void start_current_race();
    void reset_players_for_race();
    void finish_current_race();
    void load_spawn_points_for_current_race(); 
    
    bool all_players_finished_race() const;
    bool all_players_disconnected() const;
    void mark_player_finished(int player_id);
    void verificar_ganadores(); 

    GameState create_snapshot();
    
    void enviar_estado_a_jugadores() { enviar_estado(); }
    void actualizar_estado_carrera() { actualizar_logica_juego(); }

    void print_current_race_table() const;
    void print_total_standings() const;

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);
    ~GameLoop() override;

    void add_race(const std::string& city, const std::string& yaml_path);
    void set_races(std::vector<std::unique_ptr<Race>> race_configs);

    void add_player(int player_id, const std::string& name, const std::string& car_name, const std::string& car_type);
    void delete_player_from_match(int player_id);
    void set_player_ready(int player_id, bool ready);

    void begin_match(); 
    void run() override;
    void stop_match();
    bool is_alive() const override { return is_running.load(); }
    
    void print_match_info() const;
};

#endif // GAME_LOOP_H