#include "game_loop.h"
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
    players.clear();
    cars.clear();

    std::cout << "[GameLoop] Destructor: recursos liberados automáticamente" << std::endl;
}

// ==========================================================
// AGREGAR JUGADORES
// ==========================================================

void GameLoop::add_player(int player_id,
                         const std::string& name,
                         const std::string& car_name,
                         const std::string& car_type) {

    // ✅ 1. Crear Car con unique_ptr (gestión automática de memoria)
    auto car = std::make_unique<Car>(car_name, car_type);

    // TODO: Cargar stats desde config.yaml según el car_type
    // Por ahora usar valores por defecto según el tipo
    if (car_type == "sport") {
        car->load_stats(150.0f, 80.0f, 1.2f, 80.0f, 2.0f, 900.0f);
    } else if (car_type == "classic") {
        car->load_stats(120.0f, 60.0f, 1.0f, 100.0f, 1.5f, 1100.0f);
    } else if (car_type == "muscle") {
        car->load_stats(140.0f, 90.0f, 0.8f, 90.0f, 1.8f, 1200.0f);
    } else {
        // Default
        car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 1.5f, 1000.0f);
    }

    // ✅ 2. Crear Player y asignarle el Car (antes de mover)
    auto player = std::make_unique<Player>(player_id, name);
    player->setCar(car.get());  // Pasar raw pointer para que Player pueda acceder
    player->resetForNewRace();

    std::cout << "[GameLoop] ✅ Jugador agregado: " << name
              << " (ID: " << player_id << ", Auto: " << car_name
              << " [" << car_type << "], HP: " << car->getHealth()
              << ", Max Speed: " << car->getMaxSpeed() << ")" << std::endl;

    // ✅ 3. Mover ownership al mapa
    cars[player_id] = std::move(car);
    players[player_id] = std::move(player);
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
        auto car_it = cars.find(comando.player_id);
        if (car_it == cars.end()) continue;

        Car* car = car_it->second.get();

        // Aplicar comando al auto del jugador
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
    for (auto& [id, car_ptr] : cars) {
        Car* car = car_ptr.get();  // ✅ Obtener raw pointer
        if (car->isDestroyed()) continue;

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
    // for (auto& [id, car_ptr] : cars) {
    //     Car* car = car_ptr.get();
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
    //         std::cout << "[GameLoop] 🏆 " << player->getName() << " terminó la carrera!" << std::endl;
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

