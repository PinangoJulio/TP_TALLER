#ifndef SERVER_GAME_H
#define SERVER_GAME_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <map>
#include <box2d/box2d.h>

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"
#include "../../common_src/config.h"

#include "car.h"
#include "collision_handler.h"
#include "obstacle.h"
#include "map_loader.h"
#include "checkpoint_manager.h"
#include "../network/monitor.h"

#define NITRO_DURATION 12
#define SLEEP 16  // ~60 FPS

class GameLoop: public Thread {
private:
    bool is_running;
    Monitor& monitor;
    std::vector<Car> cars;
    int cars_with_nitro;
    Queue<struct Command>& game_queue;
    
    b2WorldId mundo;
    Configuracion& config;
    std::map<int, b2BodyId> player_bodies;
    std::map<int, int> player_user_data;
    std::map<int, std::string> player_car_types;  // Tipo de auto por jugador
    
    CollisionHandler collision_handler;
    ObstacleManager obstacle_manager;
    CheckpointManager checkpoint_manager;
    
    std::vector<SpawnPoint> spawn_points;
    size_t next_spawn_index;
    
    void initialize_physics();
    void create_test_obstacles(); 
    void update_physics();
    void apply_forces_from_command(const Command& cmd, b2BodyId body);
    void apply_lateral_friction(b2BodyId body);
    void clamp_velocity(int player_id, b2BodyId body);
    
    void send_nitro_on();
    void send_nitro_off();
    void process_commands();
    void simulate_cars();
    
    b2Vec2 get_next_spawn_point();
    const AtributosAuto& get_car_attributes(int player_id) const;
    void create_player_body(int player_id, const std::string& car_type);

public:
    explicit GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue, Configuracion& cfg);

    void load_map(const std::string& yaml_path);
    
    b2WorldId get_world() const { return mundo; }
    ObstacleManager& get_obstacles() { return obstacle_manager; }

    void run() override;
    void stop() override { is_running = false; }
    
    ~GameLoop();

    GameLoop(const GameLoop&) = delete;
    GameLoop& operator=(const GameLoop&) = delete;
};

#endif
