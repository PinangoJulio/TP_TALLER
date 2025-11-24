#include "game_loop.h"

GameLoop::GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue, Configuracion& cfg)
    : is_running(true), 
      monitor(monitor_ref), 
      cars_with_nitro(0), 
      game_queue(queue),
      config(cfg),
      obstacle_manager(b2_nullWorldId),
      next_spawn_index(0) {
    
    initialize_physics();
}

GameLoop::~GameLoop() {
    for (auto& [player_id, body] : player_bodies) {
        if (B2_IS_NON_NULL(body)) {
            b2DestroyBody(body);
        }
    }
    player_bodies.clear();
    
    obstacle_manager.clear();
    
    if (b2World_IsValid(mundo)) {
        b2DestroyWorld(mundo);
    }
}

void GameLoop::initialize_physics() {
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {config.obtenerGravedadX(), config.obtenerGravedadY()};
    mundo = b2CreateWorld(&mundoDef);
    
    obstacle_manager = ObstacleManager(mundo);
    collision_handler.set_obstacle_manager(&obstacle_manager);
    
    create_test_obstacles();
    
    std::cout << "[GameLoop] Box2D initialized with gravity (" 
              << config.obtenerGravedadX() << ", " 
              << config.obtenerGravedadY() << ")" << std::endl;
}

void GameLoop::create_test_obstacles() {
    obstacle_manager.create_wall(-50.0f, 0.0f, 2.0f, 100.0f);
    obstacle_manager.create_wall(50.0f, 0.0f, 2.0f, 100.0f);
    obstacle_manager.create_wall(0.0f, -50.0f, 100.0f, 2.0f);
    obstacle_manager.create_wall(0.0f, 50.0f, 100.0f, 2.0f);
    
    obstacle_manager.create_building(20.0f, 20.0f, 8.0f, 8.0f);
    obstacle_manager.create_building(-20.0f, 20.0f, 8.0f, 8.0f);
    obstacle_manager.create_building(20.0f, -20.0f, 8.0f, 8.0f);
    
    obstacle_manager.create_barrier(0.0f, 0.0f, 15.0f);
    
    std::cout << "[GameLoop] Test obstacles created" << std::endl;
}

b2Vec2 GameLoop::get_next_spawn_point() {
    if (spawn_points.empty()) {
        std::cout << "[GameLoop] Warning: No spawn points, using default (0, 0)" << std::endl;
        return {0.0f, 0.0f};
    }
    
    const SpawnPoint& spawn = spawn_points[next_spawn_index];
    next_spawn_index = (next_spawn_index + 1) % spawn_points.size();
    
    return {spawn.x, spawn.y};
}

const AtributosAuto& GameLoop::get_car_attributes(int player_id) const {
    auto it = player_car_types.find(player_id);
    if (it == player_car_types.end()) {
        return config.obtenerAtributosAuto("DEPORTIVO");
    }
    return config.obtenerAtributosAuto(it->second);
}

void GameLoop::create_player_body(int player_id, const std::string& car_type) {
    const AtributosAuto& attrs = config.obtenerAtributosAuto(car_type);
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = get_next_spawn_point();
    
    bodyDef.linearDamping = 2.0f / attrs.control;
    bodyDef.angularDamping = attrs.control * 0.5f;
    
    b2BodyId body = b2CreateBody(mundo, &bodyDef);
    
    b2Polygon box;
    if (car_type == "CAMION") {
        box = b2MakeBox(1.2f, 2.5f);
    } else if (car_type == "MOTO") {
        box = b2MakeBox(0.5f, 1.5f);
    } else {
        box = b2MakeBox(1.0f, 2.0f);
    }
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = attrs.masa;

    b2CreatePolygonShape(body, &shapeDef, &box);
    
    player_user_data[player_id] = player_id;
    b2Body_SetUserData(body, &player_user_data[player_id]);
    
    player_bodies[player_id] = body;
    player_car_types[player_id] = car_type;
    
    std::cout << "[GameLoop] Player " << player_id << " body created with car type: " 
              << car_type << std::endl;
}

void GameLoop::update_physics() {
    float timeStep = config.obtenerTiempoPaso();
    int subSteps = config.obtenerIteracionesVelocidad() + 
                   config.obtenerIteracionesPosicion();
    b2World_Step(mundo, timeStep, subSteps);
    
    for (const auto& [player_id, body] : player_bodies) {
        apply_lateral_friction(body);
        clamp_velocity(player_id, body);
    }
}

void GameLoop::apply_lateral_friction(b2BodyId body) {
    if (!B2_IS_NON_NULL(body)) return;
    
    b2Vec2 velocity = b2Body_GetLinearVelocity(body);
    b2Rot rotation = b2Body_GetRotation(body);
    float angle = b2Rot_GetAngle(rotation);
    
    // Dirección lateral del auto
    b2Vec2 lateral_direction = {-std::sin(angle), std::cos(angle)};
    
    // Velocidad lateral (drift)
    float lateral_velocity = lateral_direction.x * velocity.x + 
                            lateral_direction.y * velocity.y;
    
    // Aplicar fricción para cancelar el drift (90%)
    b2Vec2 lateral_impulse = {
        -lateral_direction.x * lateral_velocity * 0.9f,
        -lateral_direction.y * lateral_velocity * 0.9f
    };
    
    b2Body_ApplyLinearImpulseToCenter(body, lateral_impulse, true);
}

void GameLoop::clamp_velocity(int player_id, b2BodyId body) {
    if (!B2_IS_NON_NULL(body)) return;
    
    const AtributosAuto& attrs = get_car_attributes(player_id);
    
    b2Vec2 vel = b2Body_GetLinearVelocity(body);
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    
    float max_speed = attrs.velocidad_maxima;
    
    // Si tiene nitro activo, aumentar límite
    auto car_it = std::find_if(cars.begin(), cars.end(),
        [&](const Car& c) { return c.get_client_id() == player_id; });
    
    if (car_it != cars.end() && car_it->is_nitro_active()) {
        max_speed *= 1.5f;
    }
    
    // Reducir velocidad máxima si está dañado
    if (car_it != cars.end() && car_it->health < 100) {
        float health_factor = static_cast<float>(car_it->health) / 100.0f;
        if (health_factor < 0.5f) {
            max_speed *= (0.5f + health_factor);
        }
    }
    
    if (speed > max_speed) {
        float factor = max_speed / speed;
        b2Body_SetLinearVelocity(body, {vel.x * factor, vel.y * factor});
    }
}

void GameLoop::apply_forces_from_command(const Command& cmd, b2BodyId body) {
    if (!B2_IS_NON_NULL(body)) return;
    
    const AtributosAuto& attrs = get_car_attributes(cmd.player_id);
    
    switch (cmd.action) {
        case GameCommand::ACCELERATE: {
            b2Vec2 vel = b2Body_GetLinearVelocity(body);
            float current_speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
            
            // Solo acelerar si no alcanzó velocidad máxima
            if (current_speed < attrs.velocidad_maxima * 1.1f) {
                b2Rot rot = b2Body_GetRotation(body);
                float angle = b2Rot_GetAngle(rot);
                
                float accel_multiplier = 1.0f;
                auto car_it = std::find_if(cars.begin(), cars.end(),
                    [&](const Car& c) { return c.get_client_id() == cmd.player_id; });
                
                if (car_it != cars.end() && car_it->is_nitro_active()) {
                    accel_multiplier = 2.0f;
                }
                
                // Fuerza proporcional a aceleración y masa
                float accel_force = attrs.aceleracion * attrs.masa * 10.0f * accel_multiplier;
                
                b2Vec2 fuerza = {
                    std::cos(angle) * accel_force,
                    std::sin(angle) * accel_force
                };
                b2Body_ApplyForceToCenter(body, fuerza, true);
            }
            break;
        }
        
        case GameCommand::BRAKE: {
            b2Vec2 vel = b2Body_GetLinearVelocity(body);
            float brake_force = attrs.masa * 15.0f;
            
            b2Vec2 frenadoFuerza = {-vel.x * brake_force, -vel.y * brake_force};
            b2Body_ApplyForceToCenter(body, frenadoFuerza, true);
            break;
        }
        
        case GameCommand::TURN_LEFT:
            b2Body_ApplyTorque(body, -attrs.control, true);
            break;
            
        case GameCommand::TURN_RIGHT:
            b2Body_ApplyTorque(body, attrs.control, true);
            break;
            
        default:
            break;
    }
}

void GameLoop::send_nitro_on() {
    cars_with_nitro++;
    std::cout << "[GameLoop] A car hit the nitro! (Total: " << cars_with_nitro << ")" << std::endl;
    
    // TODO: Implementar cuando Monitor::broadcast esté completo
    // ServerMsg msg;
    // msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);
    // msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    // msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_ON);
    // monitor.broadcast(msg);
}

void GameLoop::send_nitro_off() {
    cars_with_nitro--;
    std::cout << "[GameLoop] A car is out of juice. (Total: " << cars_with_nitro << ")" << std::endl;
    
    // TODO: Implementar cuando Monitor::broadcast esté completo
    // ServerMsg msg;
    // msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);
    // msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    // msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_OFF);
    // monitor.broadcast(msg);
}

void GameLoop::process_commands() {
    Command cmd;
    while (game_queue.try_pop(cmd)) {
        auto it = std::find_if(cars.begin(), cars.end(),
                               [&](const Car& c) { 
                                   return c.get_client_id() == cmd.player_id;
                               });

        if (it == cars.end()) {
            const AtributosAuto& attrs = config.obtenerAtributosAuto("DEPORTIVO");
            cars.emplace_back(cmd.player_id, NITRO_DURATION, attrs.salud_base);
            it = cars.end() - 1;
        }

        if (cmd.action == GameCommand::USE_NITRO) {
            if (!it->is_nitro_active()) {
                it->activate_nitro();
                send_nitro_on();
            }
        }
        
        auto body_it = player_bodies.find(cmd.player_id);
        if (body_it == player_bodies.end()) {
            create_player_body(cmd.player_id, "DEPORTIVO");
            it->body = player_bodies[cmd.player_id];
            
            collision_handler.register_car(cmd.player_id, &(*it));
            checkpoint_manager.register_player(cmd.player_id);
        }
        
        if (it->is_alive()) {
            apply_forces_from_command(cmd, player_bodies[cmd.player_id]);
        }
    }
}

void GameLoop::simulate_cars() {
    for (auto& car: cars) {
        if (car.simulate_tick()) {
            send_nitro_off();
        }
    }
}

void GameLoop::run() {
    std::cout << "[GameLoop] Starting game loop..." << std::endl;
    while (is_running) {
        process_commands();
        update_physics();
        
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        collision_handler.process_contact_event(events);
        collision_handler.apply_pending_collisions();
        collision_handler.clear_collisions();
        
        for (const auto& [player_id, body] : player_bodies) {
            b2Vec2 pos = b2Body_GetPosition(body);
            if (checkpoint_manager.check_crossing(player_id, pos)) {
                int laps = checkpoint_manager.get_laps_completed(player_id);
                std::cout << "[GameLoop] Player " << player_id 
                          << " completed lap " << laps << "!" << std::endl;
            }
        }
        
        simulate_cars();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << "[GameLoop] Game loop stopped." << std::endl;
}

void GameLoop::load_map(const std::string& yaml_path) {
    std::cout << "[GameLoop] Loading map: " << yaml_path << std::endl;
    
    MapLoader loader(mundo, obstacle_manager);
    loader.load_map(yaml_path);
    
    spawn_points = loader.get_spawn_points();
    checkpoint_manager.load_checkpoints(loader.get_checkpoints());
    
    std::cout << "[GameLoop] Map loaded with " << spawn_points.size() 
              << " spawn points and " << checkpoint_manager.get_total_checkpoints() 
              << " checkpoints" << std::endl;
}
