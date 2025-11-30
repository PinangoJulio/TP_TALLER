#include "game_loop.h"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <yaml-cpp/yaml.h>

#include "../../common_src/config.h"
#include "race.h"


GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues)
    : is_running(false), match_finished(false), comandos(comandos), queues_players(queues),
      current_race_index(0), current_race_finished(false), spawns_loaded(false)
{
    std::cout << "[GameLoop] Constructor OK. Listo para gestionar múltiples carreras.\n";
}

GameLoop::~GameLoop() {
    is_running = false;
    players.clear();
}

// AGREGAR JUGADORES
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

    // Crear Car con unique_ptr (gestión automática de memoria)
    auto car = std::make_unique<Car>(car_name, car_type);

    //Cargar stats del auto desde config.yaml
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

// MÉTODOS DE DEBUG (ACTUALIZADOS PARA MÚLTIPLES CARRERAS)

// Métodos de visualización de información ahora están en:
// - print_current_race_table()
// - print_total_standings()
// - print_match_info()

// MÉTODOS DE CONTROL


void GameLoop::stop_match() {
    std::cout << "[GameLoop] Deteniendo partida...\n";
    is_running = false;
    match_finished = true;
}

// server_src/game/game_loop.cpp

bool GameLoop::all_players_disconnected() const {
    int connected_count = 0;
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr) continue;
        // Se considera "conectado" si no está marcado como desconectado
        if (!player_ptr->isDisconnected()) {
            ++connected_count;
            if (connected_count > 1) {
                // Si hay más de 1 jugador conectado, NO terminar la partida
                return false;
            }
        }
    }
    // Si quedan 0 o 1 jugadores conectados, devolver true (terminar partida)
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

        case GameCommand::CHEAT_MAX_SPEED: // cheat opcional no obligatorio, no lo pide enunciado
            car->setCurrentSpeed(car->getMaxSpeed());
            std::cout << "[GameLoop] CHEAT: Velocidad máxima para player " << comando.player_id
                      << std::endl;
            break;

        case GameCommand::CHEAT_TELEPORT_CHECKPOINT: // cheat opcional no obligatorio, no lo pide enunciado
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
    //aca iria lo de Box2D ???
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

GameState GameLoop::create_snapshot() {
    // Convertir map<int, unique_ptr<Player>> a vector<Player*>
    std::vector<Player*> player_list;
    for (const auto& [id, player_ptr] : players) {
        player_list.push_back(player_ptr.get());
    }

    // Crear snapshot usando el constructor
    GameState snapshot(player_list, current_city_name, current_map_yaml, is_running.load());

    // TODO: Cuando se implementen, agregar:
    // - checkpoints
    // - hints
    // - NPCs
    // - eventos

    return snapshot;
}

// ==========================================================
// GESTIÓN DE MÚLTIPLES CARRERAS
// ==========================================================

void GameLoop::add_race(const std::string& city, const std::string& yaml_path) {
    int race_number = static_cast<int>(races.size()) + 1;
    auto race = std::make_unique<Race>(city, yaml_path, race_number);
    races.push_back(std::move(race));
    std::cout << "[GameLoop] Carrera " << race_number << " agregada: " << city << "\n";
}

void GameLoop::set_races(std::vector<std::unique_ptr<Race>> race_configs) {
    races = std::move(race_configs);
    race_finish_times.resize(races.size());
    std::cout << "[GameLoop] Configuradas " << races.size() << " carreras\n";
}

void GameLoop::load_spawn_points_for_current_race() {
    spawn_points.clear();
    spawns_loaded = false;

    if (current_race_index >= races.size()) {
        std::cerr << "[GameLoop] ERROR: Índice de carrera inválido\n";
        return;
    }

    try {
        std::cout << "[GameLoop] Cargando spawn points desde " << current_map_yaml << "...\n";
        YAML::Node map_yaml = YAML::LoadFile(current_map_yaml);
        YAML::Node spawns = map_yaml["spawn_points"];

        if (
            !spawns || !spawns.IsSequence()) {
            std::cerr << "[GameLoop] WARNING: 'spawn_points' ausente en " << current_map_yaml << "\n";
        } else {
            for (const auto& node : spawns) {
                float x = node["x"].as<float>();
                float y = node["y"].as<float>();
                float angle_deg = node["angle"].as<float>();
                float angle_rad = angle_deg * static_cast<float>(M_PI) / 180.0f;
                spawn_points.emplace_back(x, y, angle_rad);
            }
            std::cout << "[GameLoop] Cargados " << spawn_points.size() << " spawn points\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] ERROR leyendo spawns: " << e.what() << "\n";
    }

    spawns_loaded = true;
}

void GameLoop::reset_players_for_race() {
    std::cout << "[GameLoop] >>> Reseteando jugadores para nueva carrera...\n";

    load_spawn_points_for_current_race();

    size_t spawn_idx = 0;
    for (auto& [id, player_ptr] : players) {
        Player* player = player_ptr.get();
        Car* car = player->getCar();

        if (!car) continue;

        // Resetear estado del jugador
        player->resetForNewRace();

        // Asignar spawn point
        float spawn_x = 0.f, spawn_y = 0.f, spawn_angle = 0.f;
        if (spawn_idx < spawn_points.size()) {
            std::tie(spawn_x, spawn_y, spawn_angle) = spawn_points[spawn_idx];
        } else {
            spawn_x = 100.f + 10.f * static_cast<float>(spawn_idx);
            spawn_y = 200.f;
            spawn_angle = 0.f;
        }

        player->setPosition(spawn_x, spawn_y);
        player->setAngle(spawn_angle);
        car->setPosition(spawn_x, spawn_y);
        car->setAngle(spawn_angle);
        car->reset();

        std::cout << "[GameLoop]   " << player->getName() << " → spawn (" << spawn_x
                  << ", " << spawn_y << ")\n";

        spawn_idx++;
    }

    std::cout << "[GameLoop] <<< Jugadores reseteados\n";
}

void GameLoop::start_current_race() {
    if (current_race_index >= races.size()) {
        std::cerr << "[GameLoop] ERROR: No hay más carreras\n";
        return;
    }

    const auto& race = races[current_race_index];
    current_map_yaml = race->get_map_path();
    current_city_name = race->get_city_name();
    current_race_finished = false;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🏁 CARRERA #" << (current_race_index + 1) << "/" << races.size()
              << " - " << current_city_name << std::string(30 - current_city_name.size(), ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Mapa: " << current_map_yaml.substr(0, 50) << std::string(50 - std::min(50UL, current_map_yaml.size()), ' ') << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    reset_players_for_race();
    race_start_time = std::chrono::steady_clock::now();
}

void GameLoop::finish_current_race() {
    std::cout << "\n[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  CARRERA #" << (current_race_index + 1) << " FINALIZADA\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    print_current_race_table();

    // Avanzar a la siguiente carrera
    current_race_index++;
    current_race_finished = true;

    if (current_race_index >= races.size()) {
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
        if (!player_ptr->isFinished() && player_ptr->isAlive()) {
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
    race_finish_times[current_race_index][player_id] = finish_time_ms;

    // Actualizar tiempo total
    total_times[player_id] += finish_time_ms;

    std::cout << "[GameLoop]  " << player->getName() << " terminó la carrera #"
              << (current_race_index + 1) << " en " << (finish_time_ms / 1000.0f) << "s\n";

    print_current_race_table();
}

void GameLoop::print_current_race_table() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  TABLA DE TIEMPOS - CARRERA #" << (current_race_index + 1) << std::string(27, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";

    const auto& times = race_finish_times[current_race_index];

    if (times.empty()) {
        std::cout << "║  Nadie ha terminado aún" << std::string(35, ' ') << "║\n";
    } else {
        // Ordenar por tiempo
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
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}
// ==========================================================
// BUCLE PRINCIPAL DEL MATCH (GESTIONA TODAS LAS CARRERAS)
// ==========================================================

void GameLoop::run() {
    is_running = true;
    match_finished = false;
    current_race_index = 0;

    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  THREAD INICIADO\n";
    std::cout << "[GameLoop]  Esperando carreras y jugadores...\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    // ✅ ESPERAR A QUE HAYA CARRERAS CONFIGURADAS (con timeout de 10 segundos)
    auto wait_start = std::chrono::steady_clock::now();

    while (is_running.load() && races.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - wait_start).count();

        std::cout << "[GameLoop]  Esperando carreras... (races=" << races.size()
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
    while (is_running.load() && !match_finished.load() && current_race_index < races.size()) {
        // Iniciar carrera actual
        reset_players_for_race();
        start_current_race();

        // Bucle de la carrera actual
        while (is_running.load() && !current_race_finished.load()) {
            // --- FASE 1: Procesar Comandos ---
            procesar_comandos();

            // --- FASE 2: Actualizar Física ---
            actualizar_fisica();

            // --- FASE 3: Detectar Colisiones ---
            detectar_colisiones();

            // --- FASE 4: Actualizar Estado de Carrera ---
            actualizar_estado_carrera();

            // --- FASE 5: Verificar si todos terminaron ---
            if (all_players_finished_race()) {
                current_race_finished = true;
            }

            // --- FASE 6: Enviar Estado a Jugadores ---
            enviar_estado_a_jugadores();

            // --- Dormir para mantener frecuencia de ticks ---
            std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP));
        }

        // Finalizar carrera actual
        finish_current_race();
    }

    std::cout << "[GameLoop]  PARTIDA FINALIZADA\n";
    std::cout << "[GameLoop] Hilo de simulación detenido correctamente.\n";
}

