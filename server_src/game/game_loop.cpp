#include "game_loop.h"

GameSimulator::GameSimulator(Monitor& monitor_ref, Queue<struct Command>& queue, Configuracion& cfg)
    : is_running(true), 
      monitor(monitor_ref), 
      cars_with_nitro(0), 
      game_queue(queue),
      config(cfg) {  
    
    initialize_physics();  
}

GameSimulator::~GameSimulator() {
    if (b2World_IsValid(mundo)) {
        b2DestroyWorld(mundo);
    }
}

void GameSimulator::initialize_physics() {
    b2WorldDef mundoDef = b2DefaultWorldDef();
    mundoDef.gravity = {config.obtenerGravedadX(), config.obtenerGravedadY()};
    mundo = b2CreateWorld(&mundoDef);
    std::cout << "[GameSimulator] Box2D initialized" << std::endl;
}

void GameSimulator::update_physics() {
    float timeStep = config.obtenerTiempoPaso();
    int subSteps = config.obtenerIteracionesVelocidad() + 
                   config.obtenerIteracionesPosicion();
    b2World_Step(mundo, timeStep, subSteps);
}

void GameSimulator::apply_forces_from_command(const Command& cmd, b2BodyId body) {
    switch (cmd.action) {
        case GameCommand::ACCELERATE: {
            b2Rot rot = b2Body_GetRotation(body);
            float angle = b2Rot_GetAngle(rot);
            b2Vec2 fuerza = {std::cos(angle) * 20.0f, std::sin(angle) * 20.0f};
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

void GameSimulator::send_nitro_on() {
    cars_with_nitro++;
    ServerMsg msg;
    msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);  
    msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_ON);  
    std::cout << "A car hit the nitro!" << std::endl;
    monitor.broadcast(msg);
}

void GameSimulator::send_nitro_off() {
    cars_with_nitro--;
    ServerMsg msg;
    msg.type = static_cast<uint8_t>(ServerMessageType::MSG_SERVER);  
    msg.cars_with_nitro = static_cast<uint16_t>(this->cars_with_nitro);
    msg.nitro_status = static_cast<uint8_t>(ServerMessageType::NITRO_OFF);  
    std::cout << "A car is out of juice." << std::endl;
    monitor.broadcast(msg);
}

void GameSimulator::process_commands() {
    Command cmd;
    while (game_queue.try_pop(cmd)) {
        auto it = std::find_if(cars.begin(), cars.end(),
                               [&](const Car& c) { 
                                   return c.get_client_id() == cmd.player_id;  
                               });

        if (it == cars.end()) {
            cars.emplace_back(cmd.player_id, NITRO_DURATION);  
            it = cars.end() - 1;
        }

        if (cmd.action == GameCommand::USE_NITRO) {  
            if (!it->is_nitro_active()) {
                it->activate_nitro();
                send_nitro_on();
            }
        }
        
        : Box2D - Crear body si no existe
        auto body_it = player_bodies.find(cmd.player_id);
        if (body_it == player_bodies.end()) {
            b2BodyDef bodyDef = b2DefaultBodyDef();
            bodyDef.type = b2_dynamicBody;
            bodyDef.position = {0.0f, 0.0f};
            bodyDef.linearDamping = 2.0f;
            
            b2BodyId body = b2CreateBody(mundo, &bodyDef);
            
            b2Polygon box = b2MakeBox(1.0f, 2.0f);
            b2ShapeDef shapeDef = b2DefaultShapeDef();
            shapeDef.density = 1.5f;
            b2CreatePolygonShape(body, &shapeDef, &box);
            
            player_bodies[cmd.player_id] = body;
            std::cout << "[Physics] Player " << cmd.player_id << " body created" << std::endl;
        }
        
        : Aplicar fuerzas de Box2D
        apply_forces_from_command(cmd, player_bodies[cmd.player_id]);
    }
}

void GameSimulator::simulate_cars() {
    for (auto& car: cars) {
        if (car.simulate_tick()) {
            send_nitro_off();
        }
    }
}

void GameSimulator::run() {
    while (is_running) {
        process_commands();
        
        update_physics();  : Actualizar Box2D
        
        simulate_cars();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }
}