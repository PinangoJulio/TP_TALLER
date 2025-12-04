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
#include "../../common_src/collision_manager.h" // IMPORTANTE
#include "car.h"
#include "player.h"

#define NITRO_DURATION 12
#define SLEEP          16 

class Race;

class GameLoop : public Thread {
private:
    
    std::atomic<bool> is_running;
    std::atomic<bool> match_finished;  
    std::atomic<bool> is_game_started;

    Queue<ComandMatchDTO>& comandos;  
    ClientMonitor& queues_players;    

    std::map<int, std::unique_ptr<Player>> players;  
    
    // Carreras
    std::vector<std::unique_ptr<Race>> races;  
    size_t current_race_index;                 
    std::atomic<bool> current_race_finished;   

    // Configuración actual
    std::string current_map_yaml;
    std::string current_city_name;
    std::vector<std::tuple<float, float, float>> spawn_points;
    bool spawns_loaded;

    // Collision Manager
    std::unique_ptr<CollisionManager> collision_manager;

    // Checkpoints y lógica interna
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

    /* ---- BOX2D v3 ----
    b2WorldId physics_world_id;
    const float TIME_STEP = 1.0f / 60.0f;
    const int32_t VELOCITY_ITERATIONS = 8;
    const int32_t POSITION_ITERATIONS = 3;

    MapLoader mapLoader;
    ObstacleManager obstacleManager;
    */

    // Tiempos
    std::chrono::steady_clock::time_point race_start_time;
    std::vector<std::map<int, uint32_t>> race_finish_times;
    std::map<int, uint32_t> total_times;

    // Métodos privados
    void load_spawn_points_for_current_race();
    void reset_players_for_race();
    void start_current_race();
    void finish_current_race();
    bool all_players_finished_race() const;
    bool all_players_disconnected() const;

    void load_checkpoints_for_current_race();
    bool check_player_crossed_checkpoint(int player_id, const Checkpoint& cp);
    void update_checkpoints();

    void procesar_comandos();
    void actualizar_fisica(); // AQUÍ SE USA EL COLLISION MANAGER
    void detectar_colisiones();
    void actualizar_estado_carrera();
    void verificar_ganadores();
    void enviar_estado_a_jugadores();

    GameState create_snapshot();

    void mark_player_finished(int player_id);
    void mark_player_finished_with_time(int player_id, uint32_t finish_time_ms);
    void print_current_race_table() const;
    void print_total_standings() const;

    /*Box2d
    void load_map_for_current_race();
    */

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues);
    
    void start_game();

    // Configuración
    void add_race(const std::string& city, const std::string& yaml_path);
    void set_races(std::vector<std::unique_ptr<Race>> race_configs);

    void add_player(int player_id, const std::string& name, const std::string& car_name,
                    const std::string& car_type);
    void delete_player_from_match(int player_id);
    void set_player_ready(int player_id, bool ready);

    void run() override;
    void stop_match();
    bool is_alive() const override { return is_running.load(); }

    void print_match_info() const;

    ~GameLoop() override;
};

#endif  // GAME_LOOP_H