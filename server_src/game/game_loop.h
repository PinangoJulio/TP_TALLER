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

// Forward declaration para evitar ciclos de inclusión
class Race;

// Constantes de simulación
#define TIME_STEP (1.0f / 60.0f)
#define SUB_STEPS 4
#define SLEEP 16 // ~60 FPS

/*
 * GameLoop:
 * - Gestiona la ejecución de una partida completa (Múltiples carreras).
 * - Integra la simulación física (Box2D) con la lógica de juego (Checkpoints).
 */
class GameLoop : public Thread { 
private:
    // ---- ESTADO GENERAL ----
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;

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
    bool all_players_disconnected() const;

    // ---- GESTIÓN DE CARRERAS (TORNEO) ----
    std::vector<std::unique_ptr<Race>> races;
    int current_race_index;
    std::atomic<bool> current_race_finished;
    
    // Configuración de la carrera actual
    std::string current_map_yaml;
    std::string current_city_name;
    bool spawns_loaded;

    // ---- TIEMPOS Y RESULTADOS ----
    std::chrono::steady_clock::time_point race_start_time;
    
    // Tiempos por carrera: [indice_carrera][id_jugador] -> tiempo_ms
    std::vector<std::map<int, uint32_t>> race_finish_times; 
    // Tabla general acumulada: [id_jugador] -> tiempo_total_ms
    std::map<int, uint32_t> total_times;

    // ---- MÉTODOS INTERNOS (LÓGICA) ----
    void procesar_comandos();
    void actualizar_fisica();      // Step de Box2D
    void detectar_colisiones();    // Eventos de contacto (vía CollisionHandler)
    void actualizar_logica_juego();// Checkpoints y vueltas
    
    void enviar_estado();
    void enviar_estado_a_jugadores() { enviar_estado(); }
    void actualizar_estado_carrera() { actualizar_logica_juego(); }

    GameState create_snapshot();

    // ---- GESTIÓN DEL CICLO DE VIDA DE LA CARRERA ----
    void start_current_race();      // Carga mapa y resetea mundo
    void reset_players_for_race();  // Crea los cuerpos físicos en los spawns
    void finish_current_race();
    bool all_players_finished_race() const;
    void mark_player_finished(int player_id);
    
    // Auxiliares de carga (usados internamente o para debug)
    void load_spawn_points_for_current_race(); 

    // ---- DEBUG PRINTING ----
    void print_current_race_table() const;
    void print_total_standings() const;
    void print_match_info() const;

public:
    // Constructor estándar (usado por Match)
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);
    ~GameLoop() override;

    // ---- CONFIGURACIÓN ----
    void add_race(const std::string& city, const std::string& yaml_path);
    void set_races(std::vector<std::unique_ptr<Race>> race_configs);

    // ---- JUGADORES ----
    void add_player(int player_id, const std::string& name, const std::string& car_name, const std::string& car_type);
    void set_player_ready(int player_id, bool ready);

    // ---- CONTROL ----
    void run() override;
    void stop_match();
    bool is_alive() const override { return is_running.load(); }
    
    // Getters útiles
    int get_current_race_index() const { return current_race_index; }
};

#endif // GAME_LOOP_H
