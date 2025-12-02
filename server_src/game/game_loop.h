#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <atomic>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <box2d/box2d.h>
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/queue.h"
#include "../../common_src/thread.h"
#include "../network/client_monitor.h"
#include "player.h"
#include "race.h"
#include "map_loader.h"
#include "obstacle.h"
#include "checkpoint_manager.h"
#include "collision_handler.h"

class Race;

#define TIME_STEP (1.0f / 60.0f)
#define SUB_STEPS 4
#define SLEEP 16 

class GameLoop: public Thread{
private:
    // ---- ESTADO DEL GAMELOOP ----
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;
    std::atomic<bool> start_game_signal;
    
    // ---- COMUNICACIÓN ----
    Queue<ComandMatchDTO>& comandos;
    ClientMonitor& queues_players;
    
    // ---- JUGADORES ----
    std::map<int, std::unique_ptr<Player>> players;
    
    // ---- CARRERAS ----
    std::vector<std::unique_ptr<Race>> races;
    int current_race_index;
    std::atomic<bool> current_race_finished;
    bool spawns_loaded;
    
    // ---- TIEMPOS ----
    std::vector<std::map<int, uint32_t>> race_finish_times;
    std::map<int, uint32_t> total_times;
    std::chrono::steady_clock::time_point race_start_time;
    
    // ---- INFORMACIÓN DE CARRERA ACTUAL ----
    std::string current_city_name;
    std::string current_map_yaml;
    
    // ---- BOX2D ----
    b2WorldId worldId;
    
    // ---- SISTEMAS DEL JUEGO ----
    MapLoader mapLoader;
    ObstacleManager obstacleManager;
    CheckpointManager checkpointManager;
    CollisionHandler collisionHandler;
    
    // ---- MÉTODOS PRIVADOS ----
    bool all_players_disconnected() const;
    bool all_players_finished_race() const;
    
    void procesar_comandos();
    void actualizar_fisica();
    void actualizar_logica_juego();
    void enviar_estado();
    
    GameState create_snapshot();
    
    void start_current_race();
    void reset_players_for_race();
    void finish_current_race();
    void mark_player_finished(int player_id);
    
    void print_current_race_table() const;
    void print_total_standings() const;
    void print_match_info() const;

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);
    ~GameLoop();
    
    // ---- GESTIÓN DE JUGADORES ----
    void add_player(int player_id, const std::string& name, 
                    const std::string& car_name, const std::string& car_type);
    void delete_player_from_match(int player_id);
    void set_player_ready(int player_id, bool ready);
    
    // ---- GESTIÓN DE CARRERAS ----
    void add_race(const std::string& city, const std::string& yaml_path);
    void set_races(std::vector<std::unique_ptr<Race>> race_configs);
    
    // ---- CONTROL DEL JUEGO ----
    void begin_match();
    void run() override;
    void stop_match();
    
    // ---- MÉTODOS LEGACY (para compatibilidad) ----
    void verificar_ganadores() {}  // Ya implementado en actualizar_logica_juego()
    void detectar_colisiones() {}  // Box2D maneja las colisiones
    void actualizar_estado_carrera() {}  // Ya implementado en actualizar_logica_juego()
    void load_spawn_points_for_current_race() {}  // Ya implementado en reset_players_for_race()
};

#endif // GAME_LOOP_H