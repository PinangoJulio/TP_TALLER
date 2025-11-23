#include "game_loop.h"
#include "../../common_src/config.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <utility>

// ==========================================================
// CONSTRUCTOR Y DESTRUCTOR
// ==========================================================

GameLoop::GameLoop(Queue<ComandMatchDTO> &comandos, ClientMonitor &queues, const std::string &yaml_path)
    : is_running(false),
      race_finished(false),
      comandos(comandos),
      queues_players(queues),
      yaml_path(yaml_path),
      total_laps(3)  // Por defecto 3 vueltas
{
    std::cout << "[GameLoop] Constructor compilado OK. Simulación lista para iniciar." << std::endl;
    std::cout << "[GameLoop] Mapa: " << yaml_path << std::endl;
}

GameLoop::~GameLoop() {
    is_running = false;

    // ✅ unique_ptr libera automáticamente la memoria
    // Al limpiar players, también se liberan sus Cars automáticamente
    players.clear();

    std::cout << "[GameLoop] Destructor: recursos liberados automáticamente" << std::endl;
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

void GameLoop::add_player(int player_id,
                         const std::string& name,
                         const std::string& car_name,
                         const std::string& car_type) {

    std::cout << "\n\n\n";
    std::cout << "***************************************************************" << std::endl;
    std::cout << "█                                                              █" << std::endl;
    std::cout << "█   GAMELOOP::ADD_PLAYER() LLAMADO                            █" << std::endl;
    std::cout << "█                                                              █" << std::endl;
    std::cout << "***************************************************************" << std::endl;
    std::cout << std::endl;

    std::cout << "[GameLoop] ══════════════════════════════════════════" << std::endl;
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
                // Auto encontrado en YAML, cargar estadísticas
                float speed = car_node["speed"].as<float>();
                float acceleration = car_node["acceleration"].as<float>();
                float handling = car_node["handling"].as<float>();
                float durability = car_node["durability"].as<float>();

                // Leer health (si existe), sino usar durability como fallback
                float health = durability;  // Default: usar durability
                if (car_node["health"]) {
                    health = car_node["health"].as<float>();
                }

                // Conversión a parámetros físicos del juego
                float max_speed = speed * 1.5f;                  // 60-90 → 90-135 km/h
                float accel_power = acceleration * 0.8f;         // 50-85 → 40-68
                float turn_rate = handling * 1.0f / 100.0f;      // 55-70 → 0.55-0.70
                // health ya está leído del YAML
                float nitro_boost = 2.0f;                        // Boost fijo para todos
                float mass = 1000.0f + (durability * 5.0f);      // 1300-1450 kg (basado en durability, NO en health)

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
            std::cerr << "[GameLoop]  Auto '" << car_name << "' no encontrado en config.yaml. "
                      << "Usando valores por defecto." << std::endl;
            car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
        }

    } catch (const std::exception& e) {
        std::cerr << "[GameLoop]  Error al cargar stats: " << e.what()
                  << ". Usando valores por defecto." << std::endl;
        car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
    }

    // ✅ 3. Crear Player y asignarle el Car
    auto player = std::make_unique<Player>(player_id, name);
    player->setCar(car.get());  // Player mantiene un puntero al Car
    player->resetForNewRace();

    // ✅ 4. Transferir ownership del Car al Player
    // IMPORTANTE: El Player ahora es dueño del Car (gestión automática)
    player->setCarOwnership(std::move(car));

    std::cout << "[GameLoop] ✅ Jugador registrado exitosamente" << std::endl;
    std::cout << "[GameLoop]   • Total jugadores: " << (players.size() + 1) << std::endl;
    std::cout << "[GameLoop] ══════════════════════════════════════════" << std::endl;

    // ✅ 5. Registrar en el mapa de jugadores
    // Ahora players[player_id] contiene TODO (Player + Car)
    players[player_id] = std::move(player);

    // ✅ 6. Imprimir TODOS los datos del jugador registrado
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
        std::cout << "[GameLoop]   ├─  ESTADÍSTICAS:" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Velocidad máxima: " << registered_car->getMaxSpeed() << " km/h" << std::endl;
        std::cout << "[GameLoop]   │  ├─ Aceleración: " << registered_car->getAcceleration() << std::endl;
        std::cout << "[GameLoop]   │  ├─ Manejo (handling): " << registered_car->getHandling() << std::endl;
        std::cout << "[GameLoop]   │  └─ Peso: " << registered_car->getWeight() << " kg" << std::endl;
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
    std::cout << "\n";
    std::cout << "[GameLoop] ══════════════════════════════════════════" << std::endl;
    std::cout << "\n";
    std::cout << std::flush;  // ✅ Forzar flush del buffer
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

    std::cout << "[GameLoop] Jugador " << player->getName()
              << " marcado como " << (ready ? "LISTO ✓" : "NO LISTO ✗") << std::endl;
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
    std::cout << "║  JUGADORES REGISTRADOS (" << players.size() << ")" << std::string(33, ' ') << "║\n";
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

    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
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

    std::cout << "[GameLoop] ═══════════════════════════════════════════════" << std::endl;
    std::cout << "[GameLoop] 🏁 CARRERA INICIADA 🏁" << std::endl;
    std::cout << "[GameLoop] ═══════════════════════════════════════════════" << std::endl;

    print_race_info();

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

    std::cout << "[GameLoop]  CARRERA FINALIZADA " << std::endl;
    std::cout << "[GameLoop] Hilo de simulación detenido correctamente." << std::endl;
}


// MÉTODOS PRIVADOS (LÓGICA DE JUEGO)


void GameLoop::procesar_comandos() {
    ComandMatchDTO comando;

    // Delta time aproximado (SLEEP ms en segundos)
    float delta_time = SLEEP / 1000.0f;

    while (comandos.try_pop(comando)) {
        // ✅ Buscar el jugador por su ID (que es el client_id)
        auto player_it = players.find(comando.player_id);
        if (player_it == players.end()) {
            std::cerr << "[GameLoop]   Comando ignorado: player_id "
                      << comando.player_id << " no encontrado" << std::endl;
            continue;
        }

        // ✅ Obtener el Player y su Car
        Player* player = player_it->second.get();
        Car* car = player->getCar();

        if (!car) {
            std::cerr << "[GameLoop]  Player " << comando.player_id
                      << " no tiene auto asignado" << std::endl;
            continue;
        }

        // ✅ Aplicar comando al auto del jugador
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

            case GameCommand::CHEAT_WIN_RACE:
                {
                    auto player_it = players.find(comando.player_id);
                    if (player_it != players.end()) {
                        player_it->second->markAsFinished();
                        std::cout << "[GameLoop] CHEAT: " << player_it->second->getName()
                                  << " ganó automáticamente" << std::endl;
                    }
                }
                break;

            case GameCommand::CHEAT_MAX_SPEED:
                car->setCurrentSpeed(car->getMaxSpeed());
                std::cout << "[GameLoop] CHEAT: Velocidad máxima para player "
                          << comando.player_id << std::endl;
                break;

            case GameCommand::CHEAT_TELEPORT_CHECKPOINT:
                // TODO: Implementar teleport a checkpoint
                std::cout << "[GameLoop] CHEAT: Teleport a checkpoint "
                          << comando.checkpoint_id << std::endl;
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

            case GameCommand::DISCONNECT:
                {
                    auto player_it = players.find(comando.player_id);
                    if (player_it != players.end()) {
                        player_it->second->disconnect();
                        std::cout << "[GameLoop] Jugador " << player_it->second->getName()
                                  << " se desconectó" << std::endl;
                    }
                }
                break;

            default:
                std::cout << "[GameLoop] Comando desconocido: "
                          << static_cast<int>(comando.command) << std::endl;
                break;
        }
    }
}

void GameLoop::actualizar_fisica() {
    // Delta time aproximado
    float delta_time = SLEEP / 1000.0f;

    // ✅ Actualizar posición de cada auto según su velocidad
    for (auto& [player_id, player_ptr] : players) {
        Player* player = player_ptr.get();
        Car* car = player->getCar();

        if (!car || car->isDestroyed()) continue;

        // Actualizar posición basado en velocidad
        float new_x = car->getX() + (car->getVelocityX() * delta_time);
        float new_y = car->getY() + (car->getVelocityY() * delta_time);

        car->setPosition(new_x, new_y);

        // Aplicar fricción para desacelerar gradualmente
        float current_speed = car->getCurrentSpeed();
        if (current_speed > 0) {
            float friction = 0.98f;  // Factor de fricción
            car->setCurrentSpeed(current_speed * friction);
            car->setVelocity(car->getVelocityX() * friction,
                           car->getVelocityY() * friction);
        }
    }

    // TODO: Cuando se implemente Box2D:
    // world->Step(timeStep, velocityIterations, positionIterations);
    // for (auto& [player_id, player_ptr] : players) {
    //     Player* player = player_ptr.get();
    //     Car* car = player->getCar();
    //     auto pos = car->getBody()->GetPosition();
    //     car->setPosition(pos.x, pos.y);
    //     car->setAngle(car->getBody()->GetAngle());
    // }
}

void GameLoop::detectar_colisiones() {
    // TODO: Detectar colisiones entre autos y con el mapa
    // Usar Box2D contact listeners o queries
}

void GameLoop::actualizar_estado_carrera() {
    // TODO: Actualizar vueltas, checkpoints, posiciones
    // for (auto& [id, player] : players) {
    //     if (checkCrossedFinishLine(player)) {
    //         player->incrementLap();
    //     }
    // }
}

void GameLoop::verificar_ganadores() {
    // TODO: Verificar si algún jugador completó todas las vueltas
    // for (auto& [id, player] : players) {
    //     if (player->getCompletedLaps() >= total_laps && !player->isFinished()) {
    //         player->markAsFinished();
    //         std::cout << "[GameLoop]  " << player->getName() << " terminó la carrera!" << std::endl;
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
    // TODO: Crear GameState y enviarlo a todos los jugadores
    // GameState state;
    // // Llenar state con información de todos los jugadores
    // for (const auto& [id, player] : players) {
    //     CarState car_state;
    //     car_state.player_id = id;
    //     car_state.pos_x = player->getX();
    //     car_state.pos_y = player->getY();
    //     car_state.angle = player->getAngle();
    //     car_state.velocity = player->getSpeed();
    //     // ... llenar más datos
    //     state.cars.push_back(car_state);
    // }
    //
    // // Enviar a todos los clientes
    // queues_players.send_to_all(state);
}

