#include "game_loop.h"
#include "race.h" 

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <thread>
#include <utility>
#include <yaml-cpp/yaml.h>
#include <algorithm> 

#include "../../common_src/config.h"

// ==========================================================
// CONSTRUCTOR Y DESTRUCTOR
// ==========================================================

GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues)
    : is_running(false), 
      match_finished(false), 
      start_game_signal(false), // Señal de inicio
      comandos(comandos), queues_players(queues),
      current_race_index(0), current_race_finished(false), 
      spawns_loaded(false)
{
    // Conectar sistemas de física
    collisionHandler.set_obstacle_manager(&obstacleManager);
    
    // Crear un mundo inicial
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);

    std::cout << "[GameLoop] Constructor OK. Listo para gestionar múltiples carreras con Box2D.\n";
}

GameLoop::~GameLoop() {
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
}

// ==========================================================
// GESTIÓN DE JUGADORES
// ==========================================================

void GameLoop::add_player(int player_id, const std::string& name, const std::string& car_name,
                          const std::string& car_type) {
    std::cout << "[GameLoop] Registrando jugador: " << name << " (" << car_name << ")\n";

    // 1. Crear el Auto (Sin cuerpo físico todavía)
    b2BodyId nullBody = b2_nullBodyId; 
    auto car = std::make_unique<Car>(car_name, car_type, nullBody);

    // 2. Cargar stats del auto desde config.yaml
    try {
        YAML::Node global_config = YAML::LoadFile("config.yaml");
        YAML::Node cars_list = global_config["cars"];

        if (cars_list && cars_list.IsSequence()) {
            bool car_found = false;
            for (const auto& car_node : cars_list) {
                if (car_node["name"].as<std::string>() == car_name) {
                    float speed = car_node["speed"].as<float>();
                    float acceleration = car_node["acceleration"].as<float>();
                    float handling = car_node["handling"].as<float>();
                    float durability = car_node["durability"].as<float>();
                    float health = car_node["health"] ? car_node["health"].as<float>() : durability;

                    // Ajustes de balanceo para Box2D
                    float max_speed = speed * 1.5f;           
                    float accel_power = acceleration * 0.8f;  
                    float turn_rate = handling * 1.0f / 100.0f; 
                    float nitro_boost = 2.0f; 
                    float mass = 1000.0f + (durability * 5.0f); 

                    car->load_stats(max_speed, accel_power, turn_rate, health, nitro_boost, mass);
                    car_found = true;
                    break;
                }
            }
            if (!car_found) {
                car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] Error stats: " << e.what() << ". Usando defaults.\n";
        car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
    }

    // 3. Crear Player y guardar
    auto player = std::make_unique<Player>(player_id, name);
    player->setCarOwnership(std::move(car));

    players[player_id] = std::move(player);
    std::cout << "[GameLoop] ✅ Jugador " << player_id << " registrado.\n";
}

void GameLoop::delete_player_from_match(int player_id) {
    auto it = players.find(player_id);
    if (it != players.end()) {
        std::cout << "[GameLoop] Eliminando jugador " << it->second->getName() << "\n";
        collisionHandler.unregister_car(player_id);
        players.erase(it);
    }
}

void GameLoop::set_player_ready(int player_id, bool ready) {
    auto it = players.find(player_id);
    if (it != players.end()) {
        it->second->setReady(ready);
    }
}

// ==========================================================
// GESTIÓN DE CARRERAS
// ==========================================================

void GameLoop::add_race(const std::string& city, const std::string& yaml_path) {
    (void)city; (void)yaml_path; // Legacy interface
}

void GameLoop::set_races(std::vector<std::unique_ptr<Race>> race_configs) {
    races = std::move(race_configs);
    race_finish_times.resize(races.size());
    std::cout << "[GameLoop] Configuradas " << races.size() << " carreras\n";
}

// ✅ CORRECCIÓN 1: Renombrado a begin_match para coincidir con header
void GameLoop::begin_match() {
    start_game_signal = true;
    std::cout << "[GameLoop] >>> SEÑAL DE INICIO RECIBIDA. Desbloqueando simulación.\n";
}

// ==========================================================
// PREPARACIÓN DE CARRERA
// ==========================================================

void GameLoop::start_current_race() {
    if (current_race_index >= static_cast<int>(races.size())) return;

    const auto& race = races[current_race_index];
    
    current_city_name = race->get_city_name();
    current_map_yaml = race->get_map_path();         
    std::string race_config_path = race->get_race_path(); 

    std::cout << "\n>>> INICIANDO CARRERA " << (current_race_index + 1) << "\n";
    std::cout << "    Base: " << current_map_yaml << "\n";
    std::cout << "    Config: " << race_config_path << "\n";

    // Reiniciar Box2D
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);

    // Cargar Mapas
    try {
        std::cout << "[GameLoop] Cargando geometría...\n";
        mapLoader.load_map(worldId, obstacleManager, current_map_yaml);
        
        std::cout << "[GameLoop] Cargando configuración...\n";
        mapLoader.load_race_config(race_config_path);
        
        checkpointManager.load_checkpoints(mapLoader.get_checkpoints());
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] ❌ ERROR cargando mapa: " << e.what() << "\n";
    }

    current_race_finished = false;
    race_start_time = std::chrono::steady_clock::now();
}

void GameLoop::reset_players_for_race() {
    std::cout << "[GameLoop] >>> Reseteando jugadores y creando cuerpos...\n";

    const auto& spawns = mapLoader.get_spawn_points();
    size_t spawn_idx = 0;

    for (auto& [id, player_ptr] : players) {
        Player* player = player_ptr.get();
        Car* car = player->getCar();
        if (!car) continue;

        float spawn_x = 100.0f, spawn_y = 100.0f, spawn_angle = 0.0f;
        if (spawn_idx < spawns.size()) {
            spawn_x = spawns[spawn_idx].x;
            spawn_y = spawns[spawn_idx].y;
            spawn_angle = spawns[spawn_idx].angle;
        } else {
            spawn_x += (spawn_idx * 10.0f);
        }

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {spawn_x, spawn_y};
        bodyDef.rotation = b2MakeRot(spawn_angle);
        bodyDef.linearDamping = 1.0f; 
        bodyDef.angularDamping = 2.0f;
        
        b2BodyId carBodyId = b2CreateBody(worldId, &bodyDef);
        
        b2Polygon box = b2MakeBox(1.0f, 2.0f);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        b2CreatePolygonShape(carBodyId, &shapeDef, &box);
        
        b2Body_SetUserData(carBodyId, reinterpret_cast<void*>(static_cast<uintptr_t>(id)));

        std::string model = car->getModelName();
        std::string type = car->getCarType();
        auto newCar = std::make_unique<Car>(model, type, carBodyId);
        
        newCar->load_stats(120.0f, 50.0f, 2.0f, 100.0f, 2.0f, 1000.0f); 

        player->setCarOwnership(std::move(newCar));
        player->resetForNewRace();

        collisionHandler.register_car(id, player->getCar());
        checkpointManager.register_player(id);

        std::cout << "[GameLoop]   Player " << id << " spawneado en (" << spawn_x << ", " << spawn_y << ")\n";
        spawn_idx++;
    }
}

// ==========================================================
// BUCLE PRINCIPAL (RUN)
// ==========================================================

void GameLoop::run() {
    is_running = true;
    match_finished = false;
    current_race_index = 0;

    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  THREAD INICIADO\n";
    std::cout << "[GameLoop]  Esperando carreras, jugadores y SEÑAL DE INICIO...\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    // Esperar señal de inicio (start_game_signal)
    while (is_running.load() && (!start_game_signal.load() || races.empty())) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (!is_running.load()) return;

    std::cout << "[GameLoop] PARTIDA INICIADA. Carreras: " << races.size() << "\n";
    print_match_info();

    while (is_running.load() && !match_finished.load() && current_race_index < static_cast<int>(races.size())) {
        
        start_current_race();
        reset_players_for_race();

        while (is_running.load() && !current_race_finished.load()) {
            auto start_time = std::chrono::steady_clock::now();

            procesar_comandos();
            actualizar_fisica();
            actualizar_logica_juego();
            enviar_estado();
            
            if (all_players_finished_race()) {
                current_race_finished = true;
            }

            std::this_thread::sleep_until(start_time + std::chrono::milliseconds(SLEEP));
        }

        finish_current_race();
    }

    std::cout << "[GameLoop] PARTIDA FINALIZADA\n";
}

// ==========================================================
// LÓGICA DE JUEGO (UPDATE)
// ==========================================================

void GameLoop::procesar_comandos() {
    ComandMatchDTO cmd;
    while (comandos.try_pop(cmd)) {
        auto it = players.find(cmd.player_id);
        if (it != players.end()) {
            Car* car = it->second->getCar();
            if (car) {
                float dt = TIME_STEP;
                switch (cmd.command) {
                    case GameCommand::ACCELERATE: car->accelerate(dt); break;
                    case GameCommand::BRAKE: car->brake(dt); break;
                    case GameCommand::TURN_LEFT: car->turn_left(dt); break;
                    case GameCommand::TURN_RIGHT: car->turn_right(dt); break;
                    case GameCommand::USE_NITRO: car->activateNitro(); break;
                    case GameCommand::DISCONNECT: 
                        it->second->setDisconnected(true);
                        std::cout << "[GameLoop] Jugador desconectado.\n";
                        if (all_players_disconnected()) stop_match();
                        break;
                    default: break;
                }
            }
        }
    }
}

void GameLoop::actualizar_fisica() {
    if (!b2World_IsValid(worldId)) return;
    b2World_Step(worldId, TIME_STEP, SUB_STEPS);
    
    b2ContactEvents events = b2World_GetContactEvents(worldId);
    collisionHandler.process_contact_event(events);
    collisionHandler.apply_pending_collisions();
    collisionHandler.clear_collisions();
}

void GameLoop::actualizar_logica_juego() {
    for (auto& [id, player] : players) {
        Car* car = player->getCar();
        if (car) {
            b2Vec2 pos = {car->getX(), car->getY()};
            if (checkpointManager.check_crossing(id, pos)) {
                std::cout << "[GameLoop] 🏁 Jugador " << id << " completó vuelta!\n";
            }
            player->setCompletedLaps(checkpointManager.get_laps_completed(id));
            
            if (player->getCompletedLaps() >= 3) { 
                mark_player_finished(id);
            }
        }
    }
}

// ==========================================================
// HELPERS Y RED
// ==========================================================

void GameLoop::enviar_estado() {
    GameState snapshot = create_snapshot();
    queues_players.broadcast(snapshot);
}

GameState GameLoop::create_snapshot() {
    std::vector<Player*> player_list;
    for (auto& p : players) player_list.push_back(p.second.get());
    return GameState(player_list, current_city_name, current_map_yaml, is_running.load());
}

void GameLoop::stop_match() {
    is_running = false;
    match_finished = true;
}

void GameLoop::finish_current_race() {
    std::cout << "[GameLoop] Carrera Finalizada.\n";
    print_current_race_table();
    current_race_index++;
    
    if (current_race_index >= static_cast<int>(races.size())) {
        match_finished = true;
        print_total_standings();
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Preparar siguiente carrera si existe
        if (current_race_index < static_cast<int>(races.size())) {
            start_current_race();
            reset_players_for_race();
        }
    }
}

bool GameLoop::all_players_finished_race() const {
    if (players.empty()) return false;
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr->isFinished() && !player_ptr->isDisconnected()) return false;
    }
    return true;
}

bool GameLoop::all_players_disconnected() const {
    int connected = 0;
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr->isDisconnected()) connected++;
    }
    return connected <= 1; // Terminar si queda 0 o 1
}

void GameLoop::mark_player_finished(int player_id) {
    auto it = players.find(player_id);
    if (it == players.end()) return;
    
    Player* p = it->second.get();
    if (p->isFinished()) return;

    p->markAsFinished();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - race_start_time).count();
    
    // ✅ CORRECCIÓN 2: Casteo seguro para evitar warning signed/unsigned
    if (static_cast<size_t>(current_race_index) < race_finish_times.size()) {
        race_finish_times[current_race_index][player_id] = elapsed;
    }
    total_times[player_id] += elapsed;
    
    std::cout << "[GameLoop] " << p->getName() << " terminó en " << (elapsed/1000.0f) << "s\n";
    print_current_race_table();
}

void GameLoop::verificar_ganadores() { } 
void GameLoop::load_spawn_points_for_current_race() { } 

void GameLoop::print_current_race_table() const {
    std::cout << "\n--- RESULTADOS CARRERA " << (current_race_index + 1) << " ---\n";
    
    // ✅ CORRECCIÓN 3: Casteo seguro
    if (static_cast<size_t>(current_race_index) < race_finish_times.size()) {
        const auto& times = race_finish_times[current_race_index];
        for(auto const& [id, time] : times) {
            std::cout << "ID: " << id << " Tiempo: " << (time/1000.0f) << "s\n";
        }
    }
}

void GameLoop::print_total_standings() const {
    std::cout << "\n--- TABLA GENERAL ---\n";
    for(auto const& [id, time] : total_times) {
        std::cout << "ID: " << id << " Total: " << (time/1000.0f) << "s\n";
    }
}

void GameLoop::print_match_info() const {
    std::cout << "--- Match Info ---\n";
    std::cout << "Carreras: " << races.size() << "\n";
    std::cout << "Jugadores: " << players.size() << "\n";
}