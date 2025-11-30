#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <map>
#include <memory>
#include <string>
#include <atomic>
#include <vector>
#include <box2d/box2d.h>

#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../network/client_monitor.h"
#include "player.h"

// Módulos
#include "map_loader.h"       
#include "collision_handler.h"
#include "checkpoint_manager.h"
#include "obstacle.h"

#define TIME_STEP (1.0f / 60.0f)
#define SUB_STEPS 4

class GameLoop { 
private:
    std::atomic<bool> is_running;
    std::atomic<bool> race_finished;

    Queue<ComandMatchDTO>& comandos;
    ClientMonitor& queues_players;
    
    // ---- BOX2D WORLD ----
    b2WorldId worldId;

    // ---- SISTEMAS ----
    ObstacleManager obstacleManager;
    MapLoader mapLoader;
    CollisionHandler collisionHandler;   
    CheckpointManager checkpointManager; 

    std::map<int, std::unique_ptr<Player>> players;

    std::string yaml_path;      // Mapa base
    std::string race_yaml_path; // Configuración de carrera
    std::string city_name;
    int total_laps;

    void procesar_comandos();
    void actualizar_fisica(); 
    void actualizar_logica_juego(); 
    void enviar_estado();
    GameState create_snapshot();

public:
    // CONSTRUCTOR ACTUALIZADO: Recibe map_path Y race_path
    GameLoop(Queue<ComandMatchDTO>& cmd, ClientMonitor& mon, 
             const std::string& map_path, const std::string& race_path);
             
    ~GameLoop();

    void start();
    void stop();
    
    void add_player(int id, const std::string& name, const std::string& car_name, const std::string& car_type);
    void set_player_ready(int id, bool ready);
    void set_total_laps(int laps) { total_laps = laps; }
    void set_city_name(const std::string& city) { city_name = city; }
    
    bool is_alive() const { return is_running; }
    void join() {} 
};

#endif