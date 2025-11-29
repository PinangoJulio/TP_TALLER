#include "game_loop.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

#include "../../common_src/config.h"

// ==========================================================
// CONSTRUCTOR Y DESTRUCTOR
// ==========================================================

// GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues,
//                    const std::string& yaml_path, const std::string& city)
//     : is_running(false), race_finished(false), comandos(comandos), queues_players(queues),
//       yaml_path(yaml_path), city_name(city), total_laps(1),
//       // Cargar mapas de colisión dinámicamente según la ciudad
//       collision_manager("assets/img/map/layers/" + to_kebab_case(city) + "/" + to_kebab_case(city) +
//                             ".png",  // Camino (Suelo)
//                         "assets/img/map/layers/" + to_kebab_case(city) + "/" + to_kebab_case(city) +
//                             "-puentes.png",                                             // Puentes
//                         "assets/img/map/layers/" + to_kebab_case(city) + "/rampas.png"  // Rampas
//       ) {
//     std::cout << "[GameLoop] Iniciado en " << city_name << ". Mapa físico cargado." << std::endl;
// }
GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues,
                   const std::string& yaml_path, const std::string& city)
    : is_running(false), race_finished(false), comandos(comandos), queues_players(queues),
      yaml_path(yaml_path), city_name(city), total_laps(1),
      collision_manager("assets/img/map/layers/" + to_kebab_case(city) + "/" + to_kebab_case(city) +
                            ".png",
                        "assets/img/map/layers/" + to_kebab_case(city) + "/puentes-" + to_kebab_case(city) +
                            ".png",
                        "assets/img/map/layers/" + to_kebab_case(city) + "/rampas-" + to_kebab_case(city) +".png"
      ) {
    
    std::cout << "[GameLoop] Iniciado en " << city_name << ". Mapa físico cargado." << std::endl;
    
    // ✅ NUEVO: Cargar configuración del mapa (spawn positions, checkpoints)
    try {
        map_config = std::make_unique<MapConfig>(yaml_path);
        std::cout << "[GameLoop] ✅ Configuración del mapa cargada: " 
                  << map_config->get_spawn_positions().size() 
                  << " spawn positions disponibles" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] ⚠️ Error cargando configuración del mapa: " 
                  << e.what() << std::endl;
        std::cerr << "[GameLoop]    Se usarán posiciones por defecto" << std::endl;
        // Crear config vacía con valores por defecto
        map_config = nullptr;
    }
}


GameLoop::~GameLoop() {
    is_running = false;
    players.clear();
}

std::string GameLoop::to_kebab_case(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    std::replace(str.begin(), str.end(), ' ', '-');
    return str;
}

// ==========================================================
// AGREGAR JUGADORES
// ==========================================================
/*
 * FLUJO DE REGISTRO DE JUGADORES:
 *
 * 1. El Receiver recibe MSG_SELECT_CAR del cliente
 * 2. MatchesMonitor registra al jugador en el Match
 * 3. Match llama a GameLoop::add_player() con:
 *    - player_id: ID único del cliente (usado para identificarlo en comandos)
 *    - name: Nombre del jugador
 *    - car_name: Nombre del auto elegido (ej: "Stallion GT")
 *    - car_type: Tipo del auto (ej: "sport")
 *
 * 4. GameLoop crea:
 *    - Un Car con stats cargadas desde config.yaml
 *    - Un Player que contiene ese Car
 *    - Los registra en el mapa players[player_id]
 *
 * 5. Durante el juego:
 *    - Los comandos llegan con player_id
 *    - Buscamos players[player_id] para obtener el Player
 *    - Accedemos a su Car para aplicar el comando
 *    - Enviamos el estado actualizado a través de queues_players
 */

void GameLoop::add_player(int player_id, const std::string& name, const std::string& car_name,
                          const std::string& car_type) {
    std::cout << "\n\n\n";
    std::cout << "***************************************************************" << std::endl;
    std::cout << "█   GAMELOOP::ADD_PLAYER() LLAMADO                            █" << std::endl;
    std::cout << "***************************************************************" << std::endl;
    std::cout << std::endl;
    std::cout << "[GameLoop] Registrando jugador..." << std::endl;
    std::cout << "[GameLoop]   • ID: " << player_id << std::endl;
    std::cout << "[GameLoop]   • Nombre: " << name << std::endl;
    std::cout << "[GameLoop]   • Auto: " << car_name << " (" << car_type << ")" << std::endl;

    // ✅ 1. Crear Car con unique_ptr (gestión automática de memoria)
    auto car = std::make_unique<Car>(car_name, car_type);

    // ✅ 2. Cargar stats del auto desde config.yaml
    try {
        // ✅ Cargar config.yaml GLOBAL (no el YAML del mapa)
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

                float health = durability;  // Default: usar durability
                if (car_node["health"]) {
                    health = car_node["health"].as<float>();
                }

                float max_speed = speed * 1.5f;              // 60-90 → 90-135 km/h
                float accel_power = acceleration * 0.8f;     // 50-85 → 40-68
                float turn_rate = handling * 1.0f / 100.0f;  // 55-70 → 0.55-0.70
                // health ya está leído del YAML
                float nitro_boost = 2.0f;  // Boost fijo para todos
                float mass = 1000.0f + (durability *
                                        5.0f);  // 1300-1450 kg (basado en durability, NO en health)

                car->load_stats(max_speed, accel_power, turn_rate, health, nitro_boost, mass);

                car_found = true;

                std::cout << "[GameLoop]   Stats cargadas desde YAML:" << std::endl;
                std::cout << "[GameLoop]      - Velocidad máxima: " << max_speed << " km/h"
                          << std::endl;
                std::cout << "[GameLoop]      - Aceleración: " << accel_power << std::endl;
                std::cout << "[GameLoop]      - Manejo: " << turn_rate << std::endl;
                std::cout << "[GameLoop]      - Salud: " << health << " HP" << std::endl;
                std::cout << "[GameLoop]      - Durabilidad: " << durability << std::endl;
                std::cout << "[GameLoop]      - Masa: " << mass << " kg" << std::endl;
                break;
            }
        }

        if (!car_found) {
            car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
        }

    } catch (const std::exception& e) {
        std::cerr << "[GameLoop]  Error al cargar stats: " << e.what()
                  << ". Usando valores por defecto." << std::endl;
        car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
    }

    // Crear Player y asignarle el Car
    auto player = std::make_unique<Player>(player_id, name);
    player->setCar(car.get());
    player->resetForNewRace();

    //  Transferir ownership del Car al Player
    // IMPORTANTE: El Player ahora es dueño del Car (gestión automática)
    player->setCarOwnership(std::move(car));

    std::cout << "[GameLoop] ✅ Jugador registrado exitosamente" << std::endl;
    std::cout << "[GameLoop]   • Total jugadores: " << (players.size() + 1) << std::endl;

    // 5. Registrar en el mapa de jugadores
    // Ahora players[player_id] contiene (Player + Car)
    players[player_id] = std::move(player);

    // Imprimir TODOS los datos del jugador registrado
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
    std::cout << "[GameLoop]   ├─ Velocidad: " << registered_player->getSpeed() << " km/h"
              << std::endl;
    std::cout << "[GameLoop]   ├─ Vueltas completadas: " << registered_player->getCompletedLaps()
              << std::endl;
    std::cout << "[GameLoop]   ├─ Checkpoint actual: " << registered_player->getCurrentCheckpoint()
              << std::endl;
    std::cout << "[GameLoop]   ├─ Posición en carrera: " << registered_player->getPositionInRace()
              << std::endl;
    std::cout << "[GameLoop]   ├─ Score: " << registered_player->getScore() << std::endl;
    std::cout << "[GameLoop]   ├─ Carrera finalizada: "
              << (registered_player->isFinished() ? "Sí" : "No") << std::endl;
    std::cout << "[GameLoop]   ├─ Desconectado: "
              << (registered_player->isDisconnected() ? "Sí" : "No") << std::endl;
    std::cout << "[GameLoop]   ├─ Listo: " << (registered_player->getIsReady() ? "Sí" : "No")
              << std::endl;
    std::cout << "[GameLoop]   └─ Vivo: " << (registered_player->isAlive() ? "Sí" : "No")
              << std::endl;
    std::cout << "\n";

    if (registered_car) {
        std::cout << "[GameLoop]  DATOS DEL AUTO:" << std::endl;
        std::cout << "[GameLoop]   ├─ Modelo: " << registered_car->getModelName() << std::endl;
        std::cout << "[GameLoop]   ├─ Tipo: " << registered_car->getCarType() << std::endl;
        std::cout << "[GameLoop]   ├─ Posición: (" << registered_car->getX() << ", "
                  << registered_car->getY() << ")" << std::endl;
        std::cout << "[GameLoop]   ├─ Ángulo: " << registered_car->getAngle() << "°" << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad actual: " << registered_car->getCurrentSpeed()
                  << " km/h" << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad X: " << registered_car->getVelocityX() << std::endl;
        std::cout << "[GameLoop]   ├─ Velocidad Y: " << registered_car->getVelocityY() << std::endl;
        std::cout << "[GameLoop]   │" << std::endl;
        std::cout << "[GameLoop]   ├─  ESTADÍSTICAS:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Velocidad máxima: " << registered_car->getMaxSpeed()
                  << " km/h" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Aceleración: " << registered_car->getAcceleration()
                  << std::endl;
        std::cout << "[GameLoop]   │  ├─ Manejo (handling): " << registered_car->getHandling()
                  << std::endl;
        std::cout << "[GameLoop]   │  └─ Peso: " << registered_car->getWeight() << " kg"
                  << std::endl;
        std::cout << "[GameLoop]   │" << std::endl;
        std::cout << "[GameLoop]   ├─  SALUD:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Salud actual: " << registered_car->getHealth() << " HP"
                  << std::endl;
        std::cout << "[GameLoop]   │  └─ Destruido: "
                  << (registered_car->isDestroyed() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │" << std::endl;
        std::cout << "[GameLoop]   ├─  ESTADO:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Nitro disponible: " << registered_car->getNitroAmount()
                  << "%" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Nitro activo: "
                  << (registered_car->isNitroActive() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │  ├─ Derrapando: "
                  << (registered_car->isDrifting() ? "Sí" : "No") << std::endl;
        std::cout << "[GameLoop]   │  └─ Colisionando: "
                  << (registered_car->isColliding() ? "Sí" : "No") << std::endl;
    } else {
        std::cout << "[GameLoop]   AUTO: No asignado" << std::endl;
    }

    std::cout << "\n";
    std::cout << "[GameLoop] RESUMEN PARTIDA:" << std::endl;
    std::cout << "[GameLoop]   ├─ Total jugadores registrados: " << players.size() << std::endl;
    std::cout << "[GameLoop]   └─ Carrera iniciada: " << (is_running.load() ? "Sí" : "No")
              << std::endl;
    std::cout << std::flush;
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

void GameLoop::reset_players_spawn_positions() {
    std::cout << "\n[GameLoop] 🏁 Colocando jugadores en spawn positions..." << std::endl;
    
    if (!map_config) {
        std::cerr << "[GameLoop] ⚠️ No hay map_config, usando posiciones por defecto" 
                  << std::endl;
        // Posiciones por defecto en línea
        float start_x = 100.0f;
        float start_y = 100.0f;
        int idx = 0;
        
        for (auto& [player_id, player] : players) {
            Car* car = player->getCar();
            if (!car) continue;
            
            float x = start_x + (idx * 40.0f);  // Separación de 40px
            float y = start_y;
            float angle = 0.0f;  // Mirando hacia arriba
            
            car->setPosition(x, y);
            car->setAngle(angle);
            car->setLevel(0);  // Nivel calle
            car->reset();  // Resetear velocidad, salud, nitro
            
            std::cout << "[GameLoop]   Player " << player_id << " -> (" 
                      << x << ", " << y << ") angle=" << angle << "°" << std::endl;
            idx++;
        }
        return;
    }
    
    // Usar spawn positions del YAML
    int player_idx = 0;
    for (auto& [player_id, player] : players) {
        Car* car = player->getCar();
        if (!car) continue;
        
        SpawnPosition spawn = map_config->get_spawn_for_player(player_idx);
        
        // Convertir ángulo de grados a radianes
        float angle_rad = spawn.angle_degrees * M_PI / 180.0f;
        
        car->setPosition(spawn.x, spawn.y);
        car->setAngle(angle_rad);
        car->setLevel(0);  // Todos empiezan en nivel calle
        car->reset();  // Resetear velocidad, salud, nitro
        
        std::cout << "[GameLoop]   Player " << player_id << " (" << player->getName() 
                  << ") -> (" << spawn.x << ", " << spawn.y << ") "
                  << "angle=" << spawn.angle_degrees << "°" << std::endl;
        
        player_idx++;
    }
    
    std::cout << "[GameLoop] ✅ " << player_idx << " jugadores colocados en el mapa" 
              << std::endl;
}
// ==========================================================
// MÉTODOS DE DEBUG
// ==========================================================

void GameLoop::print_race_info() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  GAMELOOP - CARRERA EN " << std::setw(35) << std::left << city_name << "║\n";
    std::cout << "║  Mapa: " << std::setw(50) << std::left << yaml_path.substr(0, 50) << "║\n";
    std::cout << "║  Vueltas: " << total_laps << std::string(48, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  JUGADORES REGISTRADOS (" << players.size() << ")" << std::string(33, ' ')
              << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";

    if (players.empty()) {
        std::cout << "║  [VACÍO] No hay jugadores registrados" << std::string(22, ' ') << "║\n";
    } else {
        for (const auto& [id, player_ptr] : players) {
            const Player* player = player_ptr.get();  // ✅ Obtener raw pointer
            std::cout << "║ ID: " << std::setw(3) << id << " │ ";
            std::cout << std::setw(15) << std::left << player->getName() << " │ ";
            std::cout << std::setw(20) << player->getSelectedCar();
            std::cout << " │ Vueltas: " << player->getCompletedLaps() << "/" << total_laps;
            std::cout << " ║\n";
        }
    }
}

// ==========================================================
// MÉTODOS DE CONTROL
// ==========================================================

void GameLoop::stop_race() {
    std::cout << "[GameLoop] Deteniendo carrera..." << std::endl;
    is_running = false;
    race_finished = true;
}

// ==========================================================
// BUCLE PRINCIPAL
// ==========================================================

void GameLoop::run() {
    is_running = true;
    race_start_time = std::chrono::steady_clock::now();

    std::cout << "[GameLoop] ╔═══════════════════════════════════════════════╗" << std::endl;
    std::cout << "[GameLoop] ║          CARRERA INICIADA                     ║" << std::endl;
    std::cout << "[GameLoop] ╚═══════════════════════════════════════════════╝" << std::endl;

    print_race_info();

    // ✅ NUEVO: Colocar jugadores en spawn positions antes de empezar
    reset_players_spawn_positions();

    std::cout << "[GameLoop] 🏁 ¡3... 2... 1... GO!" << std::endl;
    std::cout << std::endl;

    while (is_running.load() && !race_finished.load()) {
        // --- FASE 1: Procesar Comandos ---
        procesar_comandos();

        // --- FASE 2: Actualizar Física (Box2D) ---
        actualizar_fisica();

        // --- FASE 3: Detectar Colisiones ---
        detectar_colisiones();

        // --- FASE 4: Actualizar Estado de Carrera ---
        actualizar_estado_carrera();

        // --- FASE 5: Verificar Ganadores ---
        verificar_ganadores();

        // --- FASE 6: Enviar Estado a Jugadores ---
        enviar_estado_a_jugadores();

        // --- Dormir para mantener frecuencia de ticks ---
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
    }

    std::cout << "[GameLoop] 🏁 CARRERA FINALIZADA " << std::endl;
    std::cout << "[GameLoop] Hilo de simulación detenido correctamente." << std::endl;
}


// MÉTODOS PRIVADOS (LÓGICA DE JUEGO)

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
            // Usar speed_boost si está configurado
            car->accelerate(delta_time * comando.speed_boost);
            break;

        case GameCommand::BRAKE:
            car->brake(delta_time * comando.speed_boost);
            break;

        case GameCommand::TURN_LEFT:
            // Usar turn_intensity del comando
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
            car->setVelocity(0, 0);
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

        case GameCommand::CHEAT_MAX_SPEED:  // cheat opcional no obligatorio, no lo pide enunciado
            car->setCurrentSpeed(car->getMaxSpeed());
            std::cout << "[GameLoop] CHEAT: Velocidad máxima para player " << comando.player_id
                      << std::endl;
            break;

        case GameCommand::CHEAT_TELEPORT_CHECKPOINT:  // cheat opcional no obligatorio, no lo pide
                                                      // enunciado
            break;

        // === UPGRADES ===
        case GameCommand::UPGRADE_SPEED:
        case GameCommand::UPGRADE_ACCELERATION:
        case GameCommand::UPGRADE_HANDLING:
        case GameCommand::UPGRADE_DURABILITY:
            // TODO: Implementar sistema de upgrades
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
        } break;

        default:
            std::cout << "[GameLoop] Comando desconocido: " << static_cast<int>(comando.command)
                      << std::endl;
            break;
        }
    }
}

void GameLoop::actualizar_fisica() {
    float dt = SLEEP / 1000.0f;  // Delta time en segundos

    for (auto& [id, player] : players) {
        Car* car = player->getCar();
        if (!car)
            continue;

        // 1. Estado actual
        float current_x = car->getX();
        float current_y = car->getY();
        int current_level = car->getLevel();

        // 2. Calcular nueva posición propuesta
        float speed = car->getCurrentSpeed();
        float angle = car->getAngle();

        float vel_x = speed * std::cos(angle);
        float vel_y = speed * std::sin(angle);
        car->setVelocity(vel_x, vel_y);

        float next_x = current_x + vel_x * dt;
        float next_y = current_y + vel_y * dt;

        // Limites del mapa
        int map_w = collision_manager.GetWidth();
        int map_h = collision_manager.GetHeight();
        if (map_w > 0) {
            if (next_x < 0)
                next_x = 0;
            if (next_y < 0)
                next_y = 0;
            if (next_x >= map_w)
                next_x = map_w - 1;
            if (next_y >= map_h)
                next_y = map_h - 1;
        }

        // 3. Lógica de Niveles (Ground vs Bridge)
        bool nextHasGround = collision_manager.hasGroundLevel(next_x, next_y);
        bool nextHasBridge = collision_manager.hasBridgeLevel(next_x, next_y);
        bool didTransition = false;

        // Caso A: Estamos abajo (Nivel 0) y vemos un puente
        if (current_level == 0 && nextHasBridge) {
            if (!nextHasGround) {
                // Solo puente -> Intentar subir (Rampa de subida)
                if (collision_manager.canTransition(next_x, next_y, 0, 1)) {
                    car->setPosition(next_x, next_y);
                    car->setLevel(1);
                    didTransition = true;
                }
            } else if (nextHasGround && nextHasBridge) {
                // Hay ambos (zona de rampa visual) -> Moverse libremente
                if (!collision_manager.isWall(next_x, next_y, current_level)) {
                    car->setPosition(next_x, next_y);
                    didTransition = true;
                }
            }
        }
        // Caso B: Estamos arriba (Nivel 1) y vemos suelo
        else if (current_level == 1 && nextHasGround) {
            if (!nextHasBridge) {
                // Solo suelo -> Intentar bajar (Rampa de bajada)
                if (collision_manager.canTransition(next_x, next_y, 1, 0)) {
                    car->setPosition(next_x, next_y);
                    car->setLevel(0);
                    didTransition = true;
                }
            } else if (nextHasBridge && nextHasGround) {
                // Zona mixta
                if (!collision_manager.isWall(next_x, next_y, current_level)) {
                    car->setPosition(next_x, next_y);
                    didTransition = true;
                }
            }
        }

        // Caso C: Movimiento normal (sin cambio de nivel)
        if (!didTransition) {
            if (!collision_manager.isWall(next_x, next_y, current_level)) {
                car->setPosition(next_x, next_y);
            } else {
                // Choque con pared -> Frenar
                car->setCurrentSpeed(0);
                // Opcional: car->takeDamage(5);
            }
        }
    }
}

void GameLoop::detectar_colisiones() {
    // Detectar colisiones entre autos y con el mapa --> sin implementar
    // Usar Box2D contact listeners o queries
}

void GameLoop::actualizar_estado_carrera() {
    // Actualizar vueltas, checkpoints, posiciones  --> sin implementar
    // for (auto& [id, player] : players) {
    //     if (checkCrossedFinishLine(player)) {
    //         player->incrementLap();
    //     }
    // }
}

void GameLoop::verificar_ganadores() {
    // Verificar si algún jugador completó todas las vueltas  --> sin implementar
    // algo asi ?
    // for (auto& [id, player] : players) {
    //     if (player->getCompletedLaps() >= total_laps && !player->isFinished()) {
    //         player->markAsFinished();
    //         std::cout << "[GameLoop]  " << player->getName() << " terminó la carrera!" <<
    //         std::endl;
    //     }
    // }

    // Si todos terminaron, detener carrera
    // bool all_finished = std::all_of(players.begin(), players.end(),
    //     [](const auto& p) { return p.second->isFinished(); });
    // if (all_finished) {
    //     race_finished = true;
    // }
}

void GameLoop::enviar_estado_a_jugadores() {
    // Crear snapshot del estado actual
    GameState snapshot = create_snapshot();
    // Broadcast a todos los jugadores a través del ClientMonitor
    queues_players.broadcast(snapshot);
}

// ==========================================================
// CREAR SNAPSHOT (GAMESTATE)
// ==========================================================

GameState GameLoop::create_snapshot() {
    std::vector<InfoPlayer> players_info;
    for (const auto& [id, player_ptr] : players) {
        InfoPlayer info;
        info.player_id = player_ptr->getId();

        Car* car = player_ptr->getCar();
        if (car) {
            info.pos_x = car->getX();
            info.pos_y = car->getY();
            info.angle = car->getAngle();
            info.speed = car->getCurrentSpeed();
            info.health = car->getHealth();
            info.level = car->getLevel();  // <--- IMPORTANTE: Enviar nivel
        }
        players_info.push_back(info);
    }

    GameState state;
    state.players = players_info;
    return state;

    // // Convertir map<int, unique_ptr<Player>> a vector<Player*>
    // std::vector<Player*> player_list;
    // for (const auto& [id, player_ptr] : players) {
    //     player_list.push_back(player_ptr.get());
    // }

    // // Crear snapshot usando el constructor
    // GameState snapshot(player_list, city_name, yaml_path, total_laps, is_running.load());

    // // Cuando se implementen, agregar:
    // // - checkpoints
    // // - hints
    // // - NPCs
    // // - eventos

    // return snapshot;
}
