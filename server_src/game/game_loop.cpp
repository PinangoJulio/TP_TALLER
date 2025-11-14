#include "game_loop.h"

GameLoop::GameLoop(Monitor& monitor_ref, Queue<struct Command>& queue, Configuracion& cfg)
    : is_running(true), 
      monitor(monitor_ref), 
      cars_with_nitro(0), 
      game_queue(queue),
      config(cfg),
      obstacle_manager(b2_nullWorldId) {  // Se asignará después
    
    initialize_physics();
}

GameLoop::~GameLoop() {
    if (b2World_IsValid(mundo)) {
        b2DestroyWorld(mundo);
    }
}

void GameLoop::initialize_physics() {
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {config.obtenerGravedadX(), config.obtenerGravedadY()};
    mundo = b2CreateWorld(&mundoDef);
    
    // NUEVO: Asignar el mundo al ObstacleManager
    obstacle_manager = ObstacleManager(mundo);
    
    // NUEVO: Conectar el collision handler con el obstacle manager
    collision_handler.set_obstacle_manager(&obstacle_manager);
    
    // NUEVO: Crear obstáculos de prueba
    create_test_obstacles();
    
    std::cout << "[GameLoop] Box2D initialized with gravity (" 
              << config.obtenerGravedadX() << ", " 
              << config.obtenerGravedadY() << ")" << std::endl;
}

void GameLoop::create_test_obstacles() {
    // Crear paredes de borde (como límites de la pista)
    obstacle_manager.create_wall(-50.0f, 0.0f, 2.0f, 100.0f);  // Pared izquierda
    obstacle_manager.create_wall(50.0f, 0.0f, 2.0f, 100.0f);   // Pared derecha
    obstacle_manager.create_wall(0.0f, -50.0f, 100.0f, 2.0f);  // Pared inferior
    obstacle_manager.create_wall(0.0f, 50.0f, 100.0f, 2.0f);   // Pared superior
    
    // Crear algunos edificios en el mapa
    obstacle_manager.create_building(20.0f, 20.0f, 8.0f, 8.0f);
    obstacle_manager.create_building(-20.0f, 20.0f, 8.0f, 8.0f);
    obstacle_manager.create_building(20.0f, -20.0f, 8.0f, 8.0f);
    
    // Crear barreras en el medio
    obstacle_manager.create_barrier(0.0f, 0.0f, 15.0f);
    
    std::cout << "[GameLoop] Test obstacles created" << std::endl;
}

void GameLoop::update_physics() {
    float timeStep = config.obtenerTiempoPaso();
    int subSteps = config.obtenerIteracionesVelocidad() + 
                   config.obtenerIteracionesPosicion();
    b2World_Step(mundo, timeStep, subSteps);
}

void GameLoop::apply_forces_from_command(const Command& cmd, b2BodyId body) {
    switch (cmd.action) {
        case GameCommand::ACCELERATE: {
            b2Rot rot = b2Body_GetRotation(body);
            float angle = b2Rot_GetAngle(rot);
            
            float accel_multiplier = 1.0f;
            auto car_it = std::find_if(cars.begin(), cars.end(),
                [&](const Car& c) { return c.get_client_id() == cmd.player_id; });
            
            if (car_it != cars.end() && car_it->is_nitro_active()) {
                accel_multiplier = 2.0f;
            }
            
            b2Vec2 fuerza = {
                std::cos(angle) * 20.0f * accel_multiplier,
                std::sin(angle) * 20.0f * accel_multiplier
            };
            b2Body_ApplyForceToCenter(body, fuerza, true);
            break;
        }
        case GameCommand::BRAKE: {
            b2Vec2 vel = b2Body_GetLinearVelocity(body);
            b2Vec2 frenadoFuerza = {-vel.x * 10.0f, -vel.y * 10.0f};
            b2Body_ApplyForceToCenter(body, frenadoFuerza, true);
            break;
        }
        case GameCommand::TURN_LEFT:
            b2Body_ApplyTorque(body, -8.0f, true);
            break;
        case GameCommand::TURN_RIGHT:
            b2Body_ApplyTorque(body, 8.0f, true);
            break;
        default:
            break;
    }
}

void GameLoop::send_nitro_on() {
    cars_with_nitro++;
    std::cout << "[GameLoop] A car hit the nitro! (Total: " << cars_with_nitro << ")" << std::endl;
    
    // TODO: Descomentar cuando Monitor::broadcast esté completo
    // ServerMsg msg;
    // msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);
    // msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    // msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_ON);
    // monitor.broadcast(msg);
}

void GameLoop::send_nitro_off() {
    cars_with_nitro--;
    std::cout << "[GameLoop] A car is out of juice. (Total: " << cars_with_nitro << ")" << std::endl;
    
    // TODO: Descomentar cuando Monitor::broadcast esté completo
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
            cars.emplace_back(cmd.player_id, NITRO_DURATION, 100);  // 100 HP inicial
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
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_dynamicBody;
            bodyDef.position = {0.0f, 0.0f};
            bodyDef.linearDamping = 2.0f;
            
            b2BodyId body = b2CreateBody(mundo, &bodyDef);
            
            // NUEVO: Asignar userData para identificar el cuerpo en colisiones
            player_user_data[cmd.player_id] = cmd.player_id;
            b2Body_SetUserData(body, &player_user_data[cmd.player_id]);
            
            b2Polygon box = b2MakeBox(1.0f, 2.0f);
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 1.5f;
            b2CreatePolygonShape(body, &shapeDef, &box);
            
            player_bodies[cmd.player_id] = body;
            it->body = body;  // NUEVO: Guardar referencia al cuerpo
            
            // NUEVO: Registrar el auto en el collision handler
            collision_handler.register_car(cmd.player_id, &(*it));
            
            std::cout << "[GameLoop] Player " << cmd.player_id << " body created" << std::endl;
        }
        
        // Solo aplicar fuerzas si el auto está vivo
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
        
        // Actualizar física
        update_physics();
        
        // Obtener y procesar eventos de contacto
        b2ContactEvents events = b2World_GetContactEvents(mundo);
        collision_handler.process_contact_event(events);
        collision_handler.apply_pending_collisions();
        collision_handler.clear_collisions();
        
        simulate_cars();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
    std::cout << "[GameLoop] Game loop stopped." << std::endl;
}

