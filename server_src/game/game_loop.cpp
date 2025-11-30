#include "game_loop.h"
#include <iostream>
#include <thread>
#include <chrono>

GameLoop::GameLoop(Queue<ComandMatchDTO>& cmd, ClientMonitor& mon, const std::string& map_path)
    : is_running(false), race_finished(false), comandos(cmd), queues_players(mon), yaml_path(map_path) {
    
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f}; 
    this->worldId = b2CreateWorld(&worldDef);
    
    collisionHandler.set_obstacle_manager(&obstacleManager);

    std::cout << "[GameLoop] Cargando mapa desde: " << map_path << std::endl;
    mapLoader.load_map(worldId, obstacleManager, map_path); 
    
    checkpointManager.load_checkpoints(mapLoader.get_checkpoints()); // <--- Línea corregida

    std::cout << "[GameLoop] Mundo y Mapa inicializados." << std::endl;
}

GameLoop::~GameLoop() {
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
}

void GameLoop::add_player(int id, const std::string& name, const std::string& car_name, const std::string& car_type) {
    const auto& spawns = mapLoader.get_spawn_points();
    b2Vec2 spawnPos = { 10.0f + (id * 5.0f), 10.0f }; // Default
    float spawnAngle = 0.0f;

    if (id < (int)spawns.size()) {
        spawnPos = { spawns[id].x, spawns[id].y };
        spawnAngle = spawns[id].angle;
    }

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = spawnPos;
    bodyDef.rotation = b2MakeRot(spawnAngle);
    bodyDef.linearDamping = 1.0f;
    bodyDef.angularDamping = 2.0f;
    
    b2BodyId carBodyId = b2CreateBody(worldId, &bodyDef);
    
    b2Polygon box = b2MakeBox(2.0f, 1.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
   b2Body_SetUserData(carBodyId, reinterpret_cast<void*>(static_cast<uintptr_t>(id)));

    b2CreatePolygonShape(carBodyId, &shapeDef, &box);
    
    auto car = std::make_unique<Car>(car_name, car_type, carBodyId);
    auto player = std::make_unique<Player>(id, name);
    player->setCarOwnership(std::move(car));
    
    collisionHandler.register_car(id, player->getCar());
    checkpointManager.register_player(id); // <--- Importante para contar vueltas
    
    players[id] = std::move(player);
}

void GameLoop::start() {
    is_running = true;
    std::thread([this]() {
        while (is_running) {
            auto start_time = std::chrono::steady_clock::now();

            procesar_comandos();
            actualizar_fisica();
            actualizar_logica_juego(); 
            enviar_estado();

            std::this_thread::sleep_until(start_time + std::chrono::milliseconds(16));
        }
    }).detach();
}

void GameLoop::stop() {
    is_running = false;
}

void GameLoop::procesar_comandos() {
    ComandMatchDTO cmd;
    while (comandos.try_pop(cmd)) {
        if (players.count(cmd.player_id)) {
            Car* car = players[cmd.player_id]->getCar();
            float dt = TIME_STEP;
            switch (cmd.command) {
                case GameCommand::ACCELERATE: car->accelerate(dt); break;
                case GameCommand::BRAKE: car->brake(dt); break;
                case GameCommand::TURN_LEFT: car->turn_left(dt); break;
                case GameCommand::TURN_RIGHT: car->turn_right(dt); break;
                case GameCommand::USE_NITRO: car->activateNitro(); break;
                default: break;
            }
        }
    }
}

void GameLoop::actualizar_fisica() {
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
                std::cout << "[GameLoop] Jugador " << id << " completo una vuelta!" << std::endl;
            }
            
            player->setCompletedLaps(checkpointManager.get_laps_completed(id));
            
            if (player->getCompletedLaps() >= total_laps) {
                player->markAsFinished();
            }
        }
    }
}

void GameLoop::enviar_estado() {
    GameState snapshot = create_snapshot();
    queues_players.broadcast(snapshot);
}

GameState GameLoop::create_snapshot() {
    std::vector<Player*> player_list;
    for (auto& p : players) player_list.push_back(p.second.get());
    return GameState(player_list, city_name, yaml_path, total_laps, is_running);
}

void GameLoop::set_player_ready(int id, bool ready) {
    if(players.count(id)) players[id]->setReady(ready);
}