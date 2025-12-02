#include "game_loop.h"
#include "race.h" 
#include "map_loader.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <algorithm> 
#include <yaml-cpp/yaml.h>

#include "../../common_src/config.h"

// ==========================================================
// CONSTRUCTOR Y DESTRUCTOR
// ==========================================================

GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues)
    : is_running(false), 
      match_finished(false), 
      start_game_signal(false),
      comandos(comandos), queues_players(queues),
      current_race_index(0), current_race_finished(false), 
      spawns_loaded(false),
      worldId(b2_nullWorldId)
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
    is_running = false;
    
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
    
    players.clear();
}

// ==========================================================
// AGREGAR JUGADORES
// ==========================================================

void GameLoop::add_player(int player_id, const std::string& name, 
                          const std::string& car_name, const std::string& car_type) {
    std::cout << "\n\n\n";
    std::cout << "***************************************************************" << std::endl;
    std::cout << "█   GAMELOOP::ADD_PLAYER() LLAMADO                            █" << std::endl;
    std::cout << "***************************************************************" << std::endl;
    std::cout << std::endl;
    std::cout << "[GameLoop] Registrando jugador..." << std::endl;
    std::cout << "[GameLoop]   • ID: " << player_id << std::endl;
    std::cout << "[GameLoop]   • Nombre: " << name << std::endl;
    std::cout << "[GameLoop]   • Auto: " << car_name << " (" << car_type << ")" << std::endl;

    // 1. Crear el Auto (Sin cuerpo físico todavía)
    b2BodyId nullBody = b2_nullBodyId; 
    auto car = std::make_unique<Car>(car_name, car_type, nullBody);

    // 2. Cargar stats del auto desde config.yaml
    try {
        YAML::Node global_config = YAML::LoadFile("config.yaml");
        YAML::Node cars_list = global_config["cars"];

        if (!cars_list || !cars_list.IsSequence()) {
            throw std::runtime_error("No se pudo leer la lista de 'cars' desde config.yaml");
        }

        bool car_found = false;

        for (const auto& car_node : cars_list) {
            std::string yaml_car_name = car_node["name"].as<std::string>();

            if (yaml_car_name == car_name) {
                float speed = car_node["speed"].as<float>();
                float acceleration = car_node["acceleration"].as<float>();
                float handling = car_node["handling"].as<float>();
                float durability = car_node["durability"].as<float>();

                float health = durability;
                if (car_node["health"]) {
                    health = car_node["health"].as<float>();
                }

                float max_speed = speed * 1.5f;
                float accel_power = acceleration * 50.0f;  // Escalado para Box2D
                float turn_rate = handling * 1.0f / 100.0f;
                float nitro_boost = 2.0f;
                float mass = 1000.0f + (durability * 5.0f);

                car->load_stats(max_speed, accel_power, turn_rate, health, nitro_boost, mass);

                car_found = true;

                std::cout << "[GameLoop]   Stats cargadas desde YAML:" << std::endl;
                std::cout << "[GameLoop]      - Velocidad máxima: " << max_speed << " km/h" << std::endl;
                std::cout << "[GameLoop]      - Aceleración: " << accel_power << std::endl;
                std::cout << "[GameLoop]      - Manejo: " << turn_rate << std::endl;
                std::cout << "[GameLoop]      - Salud: " << health << " HP" << std::endl;
                std::cout << "[GameLoop]      - Durabilidad: " << durability << std::endl;
                std::cout << "[GameLoop]      - Masa: " << mass << " kg" << std::endl;
                break;
            }
        }

        if (!car_found) {
            car->load_stats(100.0f, 2500.0f, 2.0f, 100.0f, 2.0f, 1000.0f);
        }

    } catch (const std::exception& e) {
        std::cerr << "[GameLoop]  Error al cargar stats: " << e.what()
                  << ". Usando valores por defecto." << std::endl;
        car->load_stats(100.0f, 2500.0f, 2.0f, 100.0f, 2.0f, 1000.0f);
    }

    // 3. Crear Player y guardar
    auto player = std::make_unique<Player>(player_id, name);
    player->resetForNewRace();
    player->setCarOwnership(std::move(car));

    std::cout << "[GameLoop] ✅ Jugador registrado exitosamente" << std::endl;
    std::cout << "[GameLoop]   • Total jugadores: " << (players.size() + 1) << std::endl;

    players[player_id] = std::move(player);

    Player* registered_player = players[player_id].get();
    Car* registered_car = registered_player->getCar();

    std::cout << "\n";
    std::cout << "[GameLoop] ╔══════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "[GameLoop] ║          JUGADOR REGISTRADO EXITOSAMENTE           ║" << std::endl;
    std::cout << "[GameLoop] ╚══════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    std::cout << "[GameLoop] # DATOS DEL JUGADOR:" << std::endl;
    std::cout << "[GameLoop]   ├─ ID: " << registered_player->getId() << std::endl;
    std::cout << "[GameLoop]   ├─ Nombre: " << registered_player->getName() << std::endl;
    std::cout << "[GameLoop]   ├─ Posición: (" << registered_player->getX() << ", "
              << registered_player->getY() << ")" << std::endl;
    std::cout << "[GameLoop]   ├─ Ángulo: " << registered_player->getAngle() << "°" << std::endl;
    std::cout << "[GameLoop]   ├─ Velocidad: " << registered_player->getSpeed() << " km/h" << std::endl;
    std::cout << "[GameLoop]   ├─ Vueltas completadas: " << registered_player->getCompletedLaps() << std::endl;
    std::cout << "[GameLoop]   ├─ Checkpoint actual: " << registered_player->getCurrentCheckpoint() << std::endl;
    std::cout << "[GameLoop]   ├─ Posición en carrera: " << registered_player->getPositionInRace() << std::endl;
    std::cout << "[GameLoop]   ├─ Score: " << registered_player->getScore() << std::endl;
    std::cout << "[GameLoop]   ├─ Carrera finalizada: " << (registered_player->isFinished() ? "Sí" : "No") << std::endl;
    std::cout << "[GameLoop]   ├─ Desconectado: " << (registered_player->isDisconnected() ? "Sí" : "No") << std::endl;
    std::cout << "[GameLoop]   ├─ Listo: " << (registered_player->getIsReady() ? "Sí" : "No") << std::endl;
    std::cout << "[GameLoop]   └─ Vivo: " << (registered_player->isAlive() ? "Sí" : "No") << std::endl;
    std::cout << "\n";

    if (registered_car) {
        std::cout << "[GameLoop]  DATOS DEL AUTO:" << std::endl;
        std::cout << "[GameLoop]   ├─ Modelo: " << registered_car->getModelName() << std::endl;
        std::cout << "[GameLoop]   ├─ Tipo: " << registered_car->getCarType() << std::endl;
        std::cout << "[GameLoop]   ├─ Posición: (" << registered_car->getX() << ", "
                  << registered_car->getY() << ")" << std::endl;
        std::cout << "[GameLoop]   ├─ Ángulo: " << registered_car->getAngle() << "°" << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad actual: " << registered_car->getCurrentSpeed() << " km/h" << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad X: " << registered_car->getVelocityX() << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad Y: " << registered_car->getVelocityY() << std::endl;
        std::cout << "[GameLoop]   │" << std::endl;
        std::cout << "[GameLoop]   ├─  SALUD:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Salud actual: " << registered_car->getHealth() << " HP" << std::endl;
        std::cout << "[GameLoop]   │  └─ Destruido: " << (registered_car->isDestroyed() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │" << std::endl;
        std::cout << "[GameLoop]   ├─  ESTADO:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Nitro disponible: " << registered_car->getNitroAmount() << "%" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Nitro activo: " << (registered_car->isNitroActive() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │  ├─ Derrapando: " << (registered_car->isDrifting() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │  └─ Colisionando: " << (registered_car->isColliding() ? "Sí" : "No") << std::endl;
    } else {
        std::cout << "[GameLoop]   AUTO: No asignado" << std::endl;
    }

    std::cout << "\n";
    std::cout << "[GameLoop] RESUMEN PARTIDA:" << std::endl;
    std::cout << "[GameLoop]   ├─ Total jugadores registrados: " << players.size() << std::endl;
    std::cout << "[GameLoop]   └─ Carrera iniciada: " << (is_running.load() ? "Sí" : "No") << std::endl;
    std::cout << std::flush;
}

void GameLoop::delete_player_from_match(int player_id) {
    auto it = players.find(player_id);
    if (it != players.end()) {
        std::cout << "[GameLoop] Eliminando jugador " << it->second->getName() << "\n";
        collisionHandler.unregister_car(player_id);
        players.erase(it);
    }
}

// ==========================================================
// ACTUALIZAR ESTADO DE JUGADORES
// ==========================================================

void GameLoop::set_player_ready(int player_id, bool ready) {
    auto it = players.find(player_id);
    if (it == players.end()) {
        std::cerr << "[GameLoop]   Player " << player_id << " no encontrado" << std::endl;
        return;
    }

    Player* player = it->second.get();
    player->setReady(ready);
}

// ==========================================================
// MÉTODOS DE CONTROL
// ==========================================================

void GameLoop::stop_match() {
    std::cout << "[GameLoop] Deteniendo partida...\n";
    is_running = false;
    match_finished = true;
}

// ==========================================================
// GESTIÓN DE CARRERAS
// ==========================================================

void GameLoop::add_race(const std::string& city, const std::string& yaml_path) {
    // El constructor de Race espera: ciudad, mapa_path, race_path
    auto race = std::make_unique<Race>(city, yaml_path, "");
    races.push_back(std::move(race));
    std::cout << "[GameLoop] Carrera " << races.size() << " agregada: " << city << "\n";
}

void GameLoop::set_races(std::vector<std::unique_ptr<Race>> race_configs) {
    races = std::move(race_configs);
    race_finish_times.resize(races.size());
    std::cout << "[GameLoop] Configuradas " << races.size() << " carreras\n";
}

// ==========================================================
// PREPARACIÓN DE CARRERA
// ==========================================================

void GameLoop::start_current_race() {
    if (current_race_index >= static_cast<int>(races.size())) {
        std::cerr << "[GameLoop] ERROR: No hay más carreras\n";
        return;
    }

    const auto& race = races[current_race_index];
    current_map_yaml = race->get_map_path();
    current_city_name = race->get_city_name();
    std::string race_config_path = race->get_race_path(); 

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🏁 CARRERA #" << (current_race_index + 1) << "/" << races.size()
              << " - " << current_city_name << std::string(30 - current_city_name.size(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Mapa: " << current_map_yaml.substr(0, 50) << std::string(50 - std::min(50UL, current_map_yaml.size()), ' ') << "║\n";
    std::cout << "║  Config: " << race_config_path.substr(0, 49) << std::string(49 - std::min(49UL, race_config_path.size()), ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    current_race_finished = false;

    // Reiniciar Box2D
    if (b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
    }
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};
    worldId = b2CreateWorld(&worldDef);

    // Cargar Mapas
    try {
        std::cout << "[GameLoop] 🗺️  Cargando mapa: " << current_map_yaml << "\n";
        mapLoader.load_map(worldId, obstacleManager, current_map_yaml);
        std::cout << "[GameLoop] ✅ Mapa cargado exitosamente\n";
        
        if (!race_config_path.empty()) {
            std::cout << "[GameLoop] 🎯 Cargando config: " << race_config_path << "\n";
            mapLoader.load_race_config(race_config_path);
            std::cout << "[GameLoop] ✅ Config cargada exitosamente\n";
        }
        
        checkpointManager.load_checkpoints(mapLoader.get_checkpoints());
        std::cout << "[GameLoop] ✅ " << mapLoader.get_checkpoints().size() 
                  << " checkpoints cargados\n";
        std::cout << "[GameLoop] ✅ " << mapLoader.get_spawn_points().size() 
                  << " spawn points cargados\n";
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] ❌ ERROR cargando mapa: " << e.what() << "\n";
    }

    race_start_time = std::chrono::steady_clock::now();
}

void GameLoop::reset_players_for_race() {
    std::cout << "[GameLoop] >>> Reseteando jugadores para nueva carrera...\n";

    // Cargar spawns antes de usarlos
    /*if (current_race_index < static_cast<int>(races.size())) {
        std::string race_config_path = races[current_race_index]->get_race_path();
        if (!race_config_path.empty()) {
            mapLoader.load_race_config(race_config_path);
        }
    }*/
    
    const auto& spawns = mapLoader.get_spawn_points();
        std::cout << "[GameLoop] 📍 Spawn points disponibles: " << spawns.size() << "\n";

    if (spawns.empty()) {
        std::cerr << "[GameLoop] ❌ ERROR CRÍTICO: NO HAY SPAWN POINTS!\n";
        std::cerr << "[GameLoop]    Mapa actual: " << current_map_yaml << "\n";
        std::cerr << "[GameLoop]    Los autos se posicionarán en (0, 0)\n";
    } else {
        std::cout << "[GameLoop] ✅ Spawns cargados correctamente:\n";
        for (size_t i = 0; i < spawns.size(); ++i) {
            std::cout << "[GameLoop]    [" << i << "] x=" << spawns[i].x 
                      << " y=" << spawns[i].y 
                      << " angle=" << spawns[i].angle << "\n";
        }
    }

    size_t spawn_idx = 0;

    for (auto& [id, player_ptr] : players) {
        Player* player = player_ptr.get();
        Car* car = player->getCar();
        if (!car) {
            std::cerr << "[GameLoop] ERROR: Player " << player->getName() 
                      << " no tiene auto!\n";
            continue;
        }
        /*float saved_max_speed = car->getMaxSpeed();
        float saved_accel = car->getAcceleration();
        float saved_turn = car->getTurnRate();
        float saved_health = car->getHealth();
        float saved_nitro = car->getNitroAmount();
        float saved_mass = car->getMass();*/

        // Resetear estado del jugador
        player->resetForNewRace();

        // Asignar spawn point
        float spawn_x = 0.f, spawn_y = 0.f, spawn_angle = 0.f;
        if (spawn_idx < spawns.size()) {
            spawn_x = spawns[spawn_idx].x;
            spawn_y = spawns[spawn_idx].y;
            spawn_angle = spawns[spawn_idx].angle;
            std::cout << "[GameLoop] ✅ Usando spawn[" << spawn_idx << "]: (" 
                  << spawn_x << ", " << spawn_y << ")\n";
        } else {
            spawn_x = 3000.f + 100.f * static_cast<float>(spawn_idx);
            spawn_y = 2000.f;
            spawn_angle = 0.f;
        }

        std::cout << "[GameLoop]   " << player->getName() 
                  << " → spawn PÍXELES: (" << spawn_x << ", " << spawn_y
                  << ") ángulo: " << spawn_angle << " rad\n";


        // CONVERTIR A METROS solo para Box2D
        float spawn_x_m = spawn_x / PPM;
        float spawn_y_m = spawn_y / PPM;

        // Crear cuerpo físico Box2D
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_dynamicBody;
        bodyDef.position = {spawn_x_m, spawn_y_m};
        bodyDef.rotation = b2MakeRot(spawn_angle);
        bodyDef.linearDamping = 1.0f; 
        bodyDef.angularDamping = 3.0f;
        
        b2BodyId carBodyId = b2CreateBody(worldId, &bodyDef);
        
        b2Polygon box = b2MakeBox(0.5f, 0.5f);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 20.0f;

        // === FILTROS DE COLISIÓN ===
        shapeDef.filter.categoryBits = CATEGORY_CAR;
        shapeDef.filter.maskBits = CATEGORY_CAR | CATEGORY_WALL;
        
        b2ShapeId shapeId = b2CreatePolygonShape(carBodyId, &shapeDef, &box);
        b2Shape_SetFriction(shapeId, 0.5f);
        b2Shape_SetRestitution(shapeId, 0.2f);
        
        b2Body_SetUserData(carBodyId, reinterpret_cast<void*>(static_cast<uintptr_t>(id)));

        // Crear nuevo auto con cuerpo físico
        std::string model = car->getModelName();
        std::string type = car->getCarType();
        auto newCar = std::make_unique<Car>(model, type, carBodyId);
        
        newCar->load_stats(120.0f, 80.0f, 2.0f, 100.0f, 2.0f, 1000.0f); 

        player->setCarOwnership(std::move(newCar));
        player->setPosition(spawn_x, spawn_y);
        player->setAngle(spawn_angle);

        collisionHandler.register_car(id, player->getCar());
        checkpointManager.register_player(id);

        std::cout << "[GameLoop]   " << player->getName() << " → spawn (" << spawn_x
                  << ", " << spawn_y << ")\n";

        spawn_idx++;
    }

    std::cout << "[GameLoop] <<< Jugadores reseteados\n";
}

// ==========================================================
// BUCLE PRINCIPAL
// ==========================================================

void GameLoop::begin_match() {
    start_game_signal = true;
    std::cout << "[GameLoop] 🚦 SEÑAL DE INICIO RECIBIDA. Arrancando motores...\n";
}

void GameLoop::run() {
    is_running = true;
    match_finished = false;
    current_race_index = 0;

    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  THREAD INICIADO\n";
    std::cout << "[GameLoop]  Esperando carreras, jugadores y SEÑAL DE INICIO...\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    // ✅ ESPERAR A QUE HAYA CARRERAS CONFIGURADAS Y SEÑAL DE INICIO
    auto wait_start = std::chrono::steady_clock::now();

    while (is_running.load() && (races.empty() || !start_game_signal.load())) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - wait_start).count();

        std::cout << "[GameLoop]  Esperando... (carreras=" << races.size()
                  << ", señal=" << (start_game_signal.load() ? "SÍ" : "NO")
                  << ", elapsed=" << elapsed << "s)\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  PARTIDA INICIADA \n";
    std::cout << "[GameLoop]  Carreras configuradas: " << races.size() << "\n";
    std::cout << "[GameLoop]  Jugadores registrados: " << players.size() << "\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    print_match_info();
    
    // Bucle principal: iterar sobre todas las carreras
    while (is_running.load() && !match_finished.load() && current_race_index < static_cast<int>(races.size())) {
        // Iniciar carrera actual
        start_current_race();
        reset_players_for_race();

        // Bucle de la carrera actual
        while (is_running.load() && !current_race_finished.load()) {
            auto start_time = std::chrono::steady_clock::now();

            // --- FASE 1: Procesar Comandos ---
            procesar_comandos();

            // --- FASE 2: Actualizar Física ---
            actualizar_fisica();

            // --- FASE 3: Actualizar Lógica de Juego ---
            actualizar_logica_juego();

            // --- FASE 4: Verificar si todos terminaron ---
            if (all_players_finished_race()) {
                current_race_finished = true;
            }

            // --- FASE 5: Enviar Estado a Jugadores ---
            enviar_estado();

            // --- Dormir para mantener frecuencia de ticks ---
            std::this_thread::sleep_until(start_time + std::chrono::milliseconds(SLEEP));
        }

        // Finalizar carrera actual
        finish_current_race();
    }

    std::cout << "[GameLoop]  PARTIDA FINALIZADA\n";
    std::cout << "[GameLoop] Hilo de simulación detenido correctamente.\n";
}

// ==========================================================
// LÓGICA DE JUEGO
// ==========================================================

bool GameLoop::all_players_disconnected() const {
    int connected_count = 0;
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr) continue;
        if (!player_ptr->isDisconnected()) {
            ++connected_count;
            if (connected_count > 1) {
                return false;
            }
        }
    }
    return true;
}

void GameLoop::procesar_comandos() {
    ComandMatchDTO comando;

    // Delta time aproximado (SLEEP ms en segundos)
    float delta_time = SLEEP / 1000.0f;

    while (comandos.try_pop(comando)) {
        auto player_it = players.find(comando.player_id);
        if (player_it == players.end()) {
            std::cerr << "[GameLoop]   Comando ignorado: player_id " << comando.player_id
                      << " no encontrado" << std::endl;
            continue;
        }

        // Obtener el Player y su Car
        Player* player = player_it->second.get();
        Car* car = player->getCar();

        if (!car) {
            std::cerr << "[GameLoop]  Player " << comando.player_id << " no tiene auto asignado"
                      << std::endl;
            continue;
        }

        switch (comando.command) {
        case GameCommand::ACCELERATE:
            car->accelerate(delta_time * comando.speed_boost);
            break;

        case GameCommand::BRAKE:
            car->brake(delta_time * comando.speed_boost);
            break;

        case GameCommand::TURN_LEFT:
            car->turn_left(delta_time * comando.turn_intensity);
            break;

        case GameCommand::TURN_RIGHT:
            car->turn_right(delta_time * comando.turn_intensity);
            break;

        case GameCommand::USE_NITRO:
            car->activateNitro();
            break;

        case GameCommand::STOP_ALL:
            car->setCurrentSpeed(0);
            car->stop();
            break;

        // === CHEATS ===
        case GameCommand::CHEAT_INVINCIBLE:
            car->repair(1000.0f);
            std::cout << "[GameLoop] CHEAT: Invencibilidad activada para player "
                      << comando.player_id << std::endl;
            break;

        case GameCommand::CHEAT_WIN_RACE: {
            auto player_it = players.find(comando.player_id);
            if (player_it != players.end()) {
                player_it->second->markAsFinished();
                std::cout << "[GameLoop] CHEAT: " << player_it->second->getName()
                          << " ganó automáticamente" << std::endl;
            }
        } break;

        case GameCommand::CHEAT_MAX_SPEED:
            car->setCurrentSpeed(120.0f);
            std::cout << "[GameLoop] CHEAT: Velocidad máxima para player " << comando.player_id
                      << std::endl;
            break;

        case GameCommand::CHEAT_TELEPORT_CHECKPOINT:
            break;

        // === UPGRADES ===
        case GameCommand::UPGRADE_SPEED:
        case GameCommand::UPGRADE_ACCELERATION:
        case GameCommand::UPGRADE_HANDLING:
        case GameCommand::UPGRADE_DURABILITY:
            std::cout << "[GameLoop] UPGRADE solicitado: tipo="
                      << static_cast<int>(comando.upgrade_type)
                      << " nivel=" << static_cast<int>(comando.upgrade_level)
                      << " costo=" << comando.upgrade_cost_ms << "ms" << std::endl;
            break;

        case GameCommand::DISCONNECT: {
            auto player_it = players.find(comando.player_id);
            if (player_it != players.end()) {
                player_it->second->disconnect();
                std::cout << "[GameLoop] Jugador " << player_it->second->getName()
                          << " se desconectó" << std::endl;

            }
            if (all_players_disconnected()) {
                std::cout << "[GameLoop] Todos los jugadores se desconectaron. Deteniendo partida."
                          << std::endl;
                stop_match();
            }
        } break;

        default:
            std::cout << "[GameLoop] Comando desconocido: " << static_cast<int>(comando.command)
                      << std::endl;
            break;
        }
    }
}

void GameLoop::actualizar_fisica() {
    if (!b2World_IsValid(worldId)) return;

    // Aplicar fricción lateral (agarre) antes del paso físico
    for (auto& [id, player] : players) {
        if (player->getCar()) {
            player->getCar()->updatePhysics();
        }
    }

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
// COMUNICACIÓN CON CLIENTES
// ==========================================================

void GameLoop::enviar_estado() {
    GameState snapshot = create_snapshot();
    queues_players.broadcast(snapshot);
}

GameState GameLoop::create_snapshot() {
    std::vector<Player*> player_list;
    for (const auto& [id, player_ptr] : players) {
        player_list.push_back(player_ptr.get());
    }

    GameState snapshot(player_list, current_city_name, current_map_yaml, is_running.load());
    return snapshot;
}

// ==========================================================
// FINALIZACIÓN DE CARRERAS
// ==========================================================

void GameLoop::finish_current_race() {
    std::cout << "\n[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  CARRERA #" << (current_race_index + 1) << " FINALIZADA\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    print_current_race_table();

    // Avanzar a la siguiente carrera
    current_race_index++;
    current_race_finished = true;

    if (current_race_index >= static_cast<int>(races.size())) {
        std::cout << "[GameLoop]  TODAS LAS CARRERAS COMPLETADAS\n";
        print_total_standings();
        match_finished = true;
    } else {
        std::cout << "[GameLoop] Preparando siguiente carrera...\n";
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

bool GameLoop::all_players_finished_race() const {
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr->isFinished() && player_ptr->isAlive() && !player_ptr->isDisconnected()) {
            return false;
        }
    }
    return true;
}

void GameLoop::mark_player_finished(int player_id) {
    auto it = players.find(player_id);
    if (it == players.end()) return;

    Player* player = it->second.get();
    if (player->isFinished()) return;

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - race_start_time);
    uint32_t finish_time_ms = static_cast<uint32_t>(elapsed.count());

    player->markAsFinished();

    // Guardar tiempo de esta carrera
    if (current_race_index < static_cast<int>(race_finish_times.size())) {
        race_finish_times[current_race_index][player_id] = finish_time_ms;
    }

    // Actualizar tiempo total
    total_times[player_id] += finish_time_ms;

    std::cout << "[GameLoop]  " << player->getName() << " terminó la carrera #"
              << (current_race_index + 1) << " en " << (finish_time_ms / 1000.0f) << "s\n";

    print_current_race_table();
}

// ==========================================================
// MÉTODOS DE VISUALIZACIÓN
// ==========================================================

void GameLoop::print_current_race_table() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TABLA DE TIEMPOS - CARRERA #" << (current_race_index + 1) << std::string(27, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";

    if (current_race_index < static_cast<int>(race_finish_times.size())) {
        const auto& times = race_finish_times[current_race_index];

        if (times.empty()) {
            std::cout << "║  Nadie ha terminado aún" << std::string(35, ' ') << "║\n";
        } else {
            std::vector<std::pair<int, uint32_t>> sorted_times(times.begin(), times.end());
            std::sort(sorted_times.begin(), sorted_times.end(),
                      [](const auto& a, const auto& b) { return a.second < b.second; });

            int pos = 1;
            for (const auto& [player_id, time_ms] : sorted_times) {
                auto it = players.find(player_id);
                if (it == players.end()) continue;

                std::string name = it->second->getName();
                float time_s = time_ms / 1000.0f;

                std::cout << "║ " << pos << ". " << std::setw(20) << std::left << name
                          << " " << std::fixed << std::setprecision(2) << time_s << "s"
                          << std::string(20, ' ') << "║\n";
                pos++;
            }
        }
    } else {
        std::cout << "║  No hay datos disponibles" << std::string(34, ' ') << "║\n";
    }

    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}

void GameLoop::print_total_standings() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TABLA GENERAL - ACUMULADO TOTAL" << std::string(26, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";

    if (total_times.empty()) {
        std::cout << "║  No hay tiempos registrados" << std::string(31, ' ') << "║\n";
    } else {
        std::vector<std::pair<int, uint32_t>> sorted_totals(total_times.begin(), total_times.end());
        std::sort(sorted_totals.begin(), sorted_totals.end(),
                  [](const auto& a, const auto& b) { return a.second < b.second; });

        int pos = 1;
        for (const auto& [player_id, total_ms] : sorted_totals) {
            auto it = players.find(player_id);
            if (it == players.end()) continue;

            std::string name = it->second->getName();
            float total_s = total_ms / 1000.0f;

            std::cout << "║ " << pos << ". " << std::setw(20) << std::left << name
                      << " " << std::fixed << std::setprecision(2) << total_s << "s"
                      << std::string(20, ' ') << "║\n";
            pos++;
        }
    }

    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}

void GameLoop::print_match_info() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  GAMELOOP - INFORMACIÓN DE LA PARTIDA" << std::string(21, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Total de carreras: " << races.size() << std::string(37, ' ') << "║\n";
    std::cout << "║  Carrera actual: " << (current_race_index + 1) << "/" << races.size()
              << std::string(37, ' ') << "║\n";
    std::cout << "║  Jugadores: " << players.size() << std::string(45, ' ') << "║\n";
    std::cout << "║  Señal de inicio: " << (start_game_signal.load() ? "ACTIVA" : "INACTIVA")
              << std::string(32, ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}
