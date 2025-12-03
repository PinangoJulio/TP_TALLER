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

// 1. Inicializar is_game_started en false
GameLoop::GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues)
    : is_running(false), 
      match_finished(false), 
      is_game_started(false),
      comandos(comandos), 
      queues_players(queues),
      current_race_index(0), 
      current_race_finished(false), 
      spawns_loaded(false),
      physics_world_created(false)
{

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 0.0f};

    physics_world_id = b2CreateWorld(&worldDef);
    physics_world_created = b2World_IsValid(physics_world_id);

    std::cout << "[GameLoop] Constructor OK. Listo para gestionar múltiples carreras.\n";
}

GameLoop::~GameLoop() {
    is_running = false;
    players.clear();
}

// 2. Implementación del método para desbloquear el loop
void GameLoop::start_game() {
    std::cout << "[GameLoop] >>> SEÑAL DE INICIO RECIBIDA. Desbloqueando simulación.\n";
    is_game_started = true;
}

static inline bool segment_intersects_obb(float x0, float y0, float x1, float y1,
                                          float cx, float cy, float w, float h, float angle_deg) {
    const float ang = angle_deg * (M_PI / 180.0f);
    const float c = std::cos(-ang);
    const float s = std::sin(-ang);
    auto to_local = [&](float x, float y) {
        float dx = x - cx, dy = y - cy;
        return std::pair<float,float>{dx * c - dy * s, dx * s + dy * c};
    };
    auto p0 = to_local(x0, y0);
    auto p1 = to_local(x1, y1);
    float hx = w * 0.5f, hy = h * 0.5f;
    float dx = p1.first - p0.first;
    float dy = p1.second - p0.second;
    float t0 = 0.0f, t1 = 1.0f;
    auto clip = [&](float p, float q) {
        if (p == 0) return q >= 0; // paralelo
        float r = q / p;
        if (p < 0) { if (r > t1) return false; if (r > t0) t0 = r; }
        else       { if (r < t0) return false; if (r < t1) t1 = r; }
        return true;
    };
    if (!clip(-dx, p0.first + hx)) return false;
    if (!clip( dx, hx - p0.first)) return false;
    if (!clip(-dy, p0.second + hy)) return false;
    if (!clip( dy, hy - p0.second)) return false;
    return t0 <= t1;
}

void GameLoop::load_checkpoints_for_current_race() {
    checkpoints.clear();
    try {
        YAML::Node map = YAML::LoadFile(current_map_yaml);
        if (!map["checkpoints"] || !map["checkpoints"].IsSequence()) {
            return; // no prints excepto cruces
        }
        for (const auto& node : map["checkpoints"]) {
            Checkpoint cp{};
            cp.id = node["id"] ? node["id"].as<int>() : (int)checkpoints.size();
            cp.type = node["type"] ? node["type"].as<std::string>() : std::string("normal");
            cp.x = node["x"] ? node["x"].as<float>() : (node["cx"] ? node["cx"].as<float>() : 0.f);
            cp.y = node["y"] ? node["y"].as<float>() : (node["cy"] ? node["cy"].as<float>() : 0.f);
            if (node["radius"]) {
                float r = node["radius"].as<float>();
                cp.width = cp.height = 2.f * r;
            } else {
                cp.width  = node["width"]  ? node["width"].as<float>()  : 150.f;
                cp.height = node["height"] ? node["height"].as<float>() : 150.f;
            }
            cp.angle = 0.0f;
            checkpoints.push_back(std::move(cp));
        }
        std::sort(checkpoints.begin(), checkpoints.end(),
                  [](const Checkpoint& a, const Checkpoint& b) { return a.id < b.id; });
    } catch (...) { /* silencioso */ }
}

bool GameLoop::check_player_crossed_checkpoint(int player_id, const Checkpoint& cp) {
    auto it = players.find(player_id);
    if (it == players.end() || !(it->second->getCar())) return false;
    const float px = it->second->getX();
    const float py = it->second->getY();
    float w = cp.width  > 0 ? cp.width  : 150.f;
    float h = cp.height > 0 ? cp.height : 150.f;
    float scale = (cp.type == "finish") ? checkpoint_tol_finish : checkpoint_tol_base;
    w *= scale; h *= scale;
    auto prev_it = player_prev_pos.find(player_id);
    if (prev_it != player_prev_pos.end()) {
        float x0 = prev_it->second.first;
        float y0 = prev_it->second.second;
        float x1 = px;
        float y1 = py;
        float minx = cp.x - w * 0.5f;
        float maxx = cp.x + w * 0.5f;
        float miny = cp.y - h * 0.5f;
        float maxy = cp.y + h * 0.5f;
        bool p0_inside = (x0 >= minx && x0 <= maxx && y0 >= miny && y0 <= maxy);
        bool p1_inside = (x1 >= minx && x1 <= maxx && y1 >= miny && y1 <= maxy);
        if (p0_inside || p1_inside) return true;
        float seg_minx = std::min(x0, x1);
        float seg_maxx = std::max(x0, x1);
        float seg_miny = std::min(y0, y1);
        float seg_maxy = std::max(y0, y1);
        if (seg_maxx >= minx && seg_minx <= maxx && seg_maxy >= miny && seg_miny <= maxy) return true;
    }
    float minx = cp.x - w * 0.5f;
    float maxx = cp.x + w * 0.5f;
    float miny = cp.y - h * 0.5f;
    float maxy = cp.y + h * 0.5f;
    return (px >= minx && px <= maxx && py >= miny && py <= maxy);
}

void GameLoop::update_checkpoints() {
    if (checkpoints.empty()) return;
    for (auto& [pid, player_ptr] : players) {
        if (!player_ptr || player_ptr->isFinished() || player_ptr->isDisconnected()) continue;
        if (!player_next_checkpoint.count(pid)) {
            int first_idx = 0;
            for (size_t i = 0; i < checkpoints.size(); ++i) {
                if (checkpoints[i].type != "start") { first_idx = (int)i; break; }
            }
            player_next_checkpoint[pid] = first_idx;
        }
        int next_idx = player_next_checkpoint[pid];
        if (next_idx < 0 || next_idx >= (int)checkpoints.size()) continue;
        // 1) Intentar el esperado
        if (check_player_crossed_checkpoint(pid, checkpoints[next_idx])) {
            const auto& cp = checkpoints[next_idx];
            player_ptr->setCheckpoint(cp.id);

            if (cp.type == "finish") {
                mark_player_finished(pid);
            } else {
                player_next_checkpoint[pid] = std::min(next_idx + 1, (int)checkpoints.size() - 1);
            }
            continue;
        }
        // 2) Escaneo completo hacia adelante: buscar el primer cp cruzado en el resto
        bool advanced = false;
        for (int k = next_idx + 1; k < (int)checkpoints.size(); ++k) {
            const auto& cp_k = checkpoints[k];
            if (check_player_crossed_checkpoint(pid, cp_k)) {
                player_ptr->setCheckpoint(cp_k.id);

                if (cp_k.type == "finish") {
                    mark_player_finished(pid);
                } else {
                    player_next_checkpoint[pid] = std::min(k + 1, (int)checkpoints.size() - 1);
                }
                advanced = true;
                break;
            }
        }
        (void)advanced;
    }
}

void GameLoop::run() {
    is_running = true;
    match_finished = false;
    current_race_index = 0;

    // A) ESPERAR A QUE HAYA CARRERAS CONFIGURADAS
    auto wait_start = std::chrono::steady_clock::now();
    while (is_running.load() && races.empty()) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - wait_start).count();
        if (elapsed > 0 && elapsed % 5 == 0) {
             std::cout << "[GameLoop]  Esperando configuración de carreras...\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "[GameLoop] Carreras listas. PAUSADO esperando señal de inicio (start_game)...\n";
    while (is_running.load() && !is_game_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!is_running.load()) return;

    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";
    std::cout << "[GameLoop]  PARTIDA INICIADA - SIMULACIÓN COMENZANDO \n";
    std::cout << "[GameLoop]  Carreras configuradas: " << races.size() << "\n";
    std::cout << "[GameLoop]  Jugadores registrados: " << players.size() << "\n";
    std::cout << "[GameLoop] ═══════════════════════════════════════════════\n";

    print_match_info();

    // 🔧 CORRECCIÓN: Inicializar variables de mapa ANTES de resetear jugadores
    if (!races.empty()) {
        // 1. Obtener datos de la primera carrera
        const auto& first_race = races[0];
        current_map_yaml = first_race->get_map_path();
        current_city_name = first_race->get_city_name();
        current_race_finished = false;
        
        // 2. Resetear jugadores con las posiciones spawn del YAML
        reset_players_for_race();
        
        // 3. Marcar inicio oficial de tiempos
        race_start_time = std::chrono::steady_clock::now();
        std::cout << "[GameLoop]   Cronómetro iniciado\n";
    }
    // Loop por cada carrera (ronda)
    while (is_running.load() && !match_finished.load() && current_race_index < races.size()) {


        auto next_frame = std::chrono::steady_clock::now();
        const auto frame_duration = std::chrono::milliseconds(SLEEP);  // 16ms = ~60 FPS

        while (is_running.load() && !current_race_finished.load()) {

            procesar_comandos();

            actualizar_fisica();
            detectar_colisiones();
            actualizar_estado_carrera();

          
            update_checkpoints();

            if (all_players_finished_race()) {
                current_race_finished = true;
            }

            enviar_estado_a_jugadores();

          
            for (auto& [id, p] : players) {
                player_prev_pos[id] = {p->getX(), p->getY()};
            }

            next_frame += frame_duration;
            std::this_thread::sleep_until(next_frame);

            auto now = std::chrono::steady_clock::now();
            if (now > next_frame) {
                next_frame = now;
            }
        }
        finish_current_race();
    }

    std::cout << "[GameLoop]  PARTIDA FINALIZADA\n";
    std::cout << "[GameLoop] Hilo de simulación detenido correctamente.\n";
}

// --------------------------------------------------------
// IMPLEMENTACIÓN DE MÉTODOS AUXILIARES
// --------------------------------------------------------

void GameLoop::add_player(int player_id, const std::string& name, const std::string& car_name,
                          const std::string& car_type) {
    auto car = std::make_unique<Car>(car_name, car_type);

    // Cargar stats
    try {
        YAML::Node global_config = YAML::LoadFile("config.yaml");
        YAML::Node cars_list = global_config["cars"];
        float speed_scale = 1.0f;
        float accel_scale = 1.0f;
        if (global_config["vehicle_speed_scale"]) speed_scale = global_config["vehicle_speed_scale"].as<float>();
        if (global_config["vehicle_accel_scale"]) accel_scale = global_config["vehicle_accel_scale"].as<float>();
        bool car_found = false;
        if (cars_list && cars_list.IsSequence()) {
            for (const auto& car_node : cars_list) {
                if (car_node["name"].as<std::string>() == car_name) {
                    float speed = car_node["speed"].as<float>();
                    float acceleration = car_node["acceleration"].as<float>();
                    float handling = car_node["handling"].as<float>();
                    float durability = car_node["durability"].as<float>();
                    float health = car_node["health"] ? car_node["health"].as<float>() : durability;
                    float max_speed = speed * speed_scale * 1.5f;
                    float accel_power = acceleration * accel_scale * 0.8f;
                    float turn_rate = handling * 1.0f / 100.0f;
                    float nitro_boost = 2.0f;
                    float mass = 1000.0f + (durability * 5.0f);
                    car->load_stats(max_speed, accel_power, turn_rate, health, nitro_boost, mass);
                    car_found = true;
                    break;
                }
            }
        }
        if (!car_found) car->load_stats(100.0f * speed_scale, 50.0f * accel_scale, 1.0f, 100.0f, 2.0f, 1000.0f);
    } catch (...) {
        // Valores por defecto con escalas
        car->load_stats(100.0f, 50.0f, 1.0f, 100.0f, 2.0f, 1000.0f);
    }

    auto player = std::make_unique<Player>(player_id, name);
    player->setCar(car.get());
    player->resetForNewRace();
    player->setCarOwnership(std::move(car));

    players[player_id] = std::move(player);
    std::cout << "[GameLoop] Jugador agregado: " << name << " (ID: " << player_id << ")\n";
}

void GameLoop::delete_player_from_match(int player_id) {
    auto it = players.find(player_id);
    if (it != players.end()) {
        std::cout << "[GameLoop] Eliminando jugador ID: " << player_id << "\n";
        players.erase(it);
    }
}

void GameLoop::set_player_ready(int player_id, bool ready) {
    auto it = players.find(player_id);
    if (it != players.end()) {
        it->second->setReady(ready);
    }
}

void GameLoop::stop_match() {
    std::cout << "[GameLoop] Deteniendo partida...\n";
    is_running = false;
    match_finished = true;
}

bool GameLoop::all_players_disconnected() const {
    int connected_count = 0;
    for (const auto& [id, player_ptr] : players) {
        if (!player_ptr->isDisconnected()) connected_count++;
    }
    return connected_count <= 1; // Terminar si queda 0 o 1 solo
}

void GameLoop::procesar_comandos() {
    ComandMatchDTO comando;
    float delta_time = SLEEP / 1000.0f;

    while (comandos.try_pop(comando)) {
        auto it = players.find(comando.player_id);
        if (it == players.end()) continue;

        Player* player = it->second.get();
        Car* car = player->getCar();
        if (!car) continue;

        switch (comando.command) {
            case GameCommand::ACCELERATE: car->accelerate(delta_time * comando.speed_boost); break;
            case GameCommand::BRAKE: car->brake(delta_time * comando.speed_boost); break;
            case GameCommand::TURN_LEFT: car->turn_left(delta_time * comando.turn_intensity); break;
            case GameCommand::TURN_RIGHT: car->turn_right(delta_time * comando.turn_intensity); break;

            case GameCommand::MOVE_UP: car->move_up(delta_time); break;
            case GameCommand::MOVE_DOWN: car->move_down(delta_time); break;
            case GameCommand::MOVE_LEFT: car->move_left(delta_time); break;
            case GameCommand::MOVE_RIGHT: car->move_right(delta_time); break;

            case GameCommand::USE_NITRO: car->activateNitro(); break;
            case GameCommand::DISCONNECT: player->disconnect(); break;
            case GameCommand::STOP_ALL: car->setCurrentSpeed(0); car->setVelocity(0,0); break;

            case GameCommand::CHEAT_INVINCIBLE: car->repair(1000.0f); break;
            case GameCommand::CHEAT_MAX_SPEED: car->setCurrentSpeed(car->getMaxSpeed()); break;

            case GameCommand::CHEAT_WIN_RACE: {
                int finish_idx = -1;
                for (size_t i = 0; i < checkpoints.size(); ++i) {
                    if (checkpoints[i].type == "finish") { finish_idx = (int)i; break; }
                }
                if (finish_idx != -1) {
                    const auto& cp_finish = checkpoints[finish_idx];
                    player->setPosition(cp_finish.x, cp_finish.y);
                    player->getCar()->setPosition(cp_finish.x, cp_finish.y);
                    player->setCheckpoint(cp_finish.id);
                    player_prev_pos[comando.player_id] = {cp_finish.x, cp_finish.y};

                    // Ganador por cheat: tiempo 0
                    mark_player_finished_with_time(comando.player_id, 0);
                    player_next_checkpoint[comando.player_id] = std::min(finish_idx + 1, (int)checkpoints.size() - 1);

                    // Cerrar carrera: marcar tiempos reales actuales para el resto
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - race_start_time);
                    uint32_t t_ms = static_cast<uint32_t>(elapsed.count());
                    for (auto& [other_id, other_player] : players) {
                        if (other_id == comando.player_id) continue;
                        if (!other_player->isFinished() && !other_player->isDisconnected()) {
                            mark_player_finished_with_time(other_id, t_ms);
                        }
                    }
                    // El loop principal detectará all_players_finished_race() y cerrará la carrera
                } else {
                    // Sin finish, aplicar mismo criterio
                    mark_player_finished_with_time(comando.player_id, 0);
                    auto now = std::chrono::steady_clock::now();
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - race_start_time);
                    uint32_t t_ms = static_cast<uint32_t>(elapsed.count());
                    for (auto& [other_id, other_player] : players) {
                        if (other_id == comando.player_id) continue;
                        if (!other_player->isFinished() && !other_player->isDisconnected()) {
                            mark_player_finished_with_time(other_id, t_ms);
                        }
                    }
                }
                break;
            }

            default: break;
        }
    }
}

void GameLoop::actualizar_fisica() { 
    if (!physics_world_created) return;
    
    b2World_Step(physics_world_id, TIME_STEP, VELOCITY_ITERATIONS);
    
    // Sincronizar todos los autos
    for (auto& [id, player] : players) {
        Car* car = player->getCar();
        if (car && car->hasPhysicsBody()) {
            car->syncFromPhysics();
        }
    }
}
void GameLoop::detectar_colisiones() { /* Collision logic here */ }

void GameLoop::actualizar_estado_carrera() { /* Checkpoints logic here */ }

void GameLoop::verificar_ganadores() { /* Win condition logic here */ }

void GameLoop::enviar_estado_a_jugadores() {
    GameState snapshot = create_snapshot();
    queues_players.broadcast(snapshot);
}

GameState GameLoop::create_snapshot() {
    std::vector<Player*> player_list;
    for (const auto& [id, player_ptr] : players) {
        player_list.push_back(player_ptr.get());
    }
    return GameState(player_list, current_city_name, current_map_yaml, is_running.load());
}

void GameLoop::add_race(const std::string& city, const std::string& yaml_path) {
    int num = (int)races.size() + 1;
    races.push_back(std::make_unique<Race>(city, yaml_path, num));
}

void GameLoop::set_races(std::vector<std::unique_ptr<Race>> race_configs) {
    races = std::move(race_configs);
    race_finish_times.resize(races.size());
}

void GameLoop::load_spawn_points_for_current_race() {
    spawn_points.clear();

    std::cout << "[GameLoop] >>> Cargando spawns desde: " << current_map_yaml << std::endl;

    try {
        YAML::Node map = YAML::LoadFile(current_map_yaml);
        if (map["spawn_points"] && map["spawn_points"].IsSequence()) {
            for (const auto& node : map["spawn_points"]) {
                float x = node["x"].as<float>();
                float y = node["y"].as<float>();
                float a = node["angle"].as<float>() * (M_PI / 180.0f);
                spawn_points.emplace_back(x, y, a);
            }
        } else {
            std::cout << "[GameLoop]  No se encontraron spawn points en el YAML." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop]  Error cargando spawns de " << current_map_yaml << ": " << e.what() << std::endl;
    }
}

void GameLoop::reset_players_for_race() {
    load_spawn_points_for_current_race();
    load_checkpoints_for_current_race();
    
    try {
        YAML::Node cfg = YAML::LoadFile("config.yaml");
        if (cfg["checkpoint_tolerance_base"]) checkpoint_tol_base = cfg["checkpoint_tolerance_base"].as<float>();
        if (cfg["checkpoint_tolerance_finish"]) checkpoint_tol_finish = cfg["checkpoint_tolerance_finish"].as<float>();
        if (cfg["checkpoint_lookahead"]) checkpoint_lookahead = cfg["checkpoint_lookahead"].as<int>();
        if (cfg["checkpoint_debug_enabled"]) checkpoint_debug_enabled = cfg["checkpoint_debug_enabled"].as<bool>();
    } catch (...) {}
    
    player_next_checkpoint.clear();
    player_prev_pos.clear();
    
    if (!checkpoints.empty()) {
        int first_idx = 0;
        for (size_t i = 0; i < checkpoints.size(); ++i) {
            if (checkpoints[i].type != "start") { first_idx = (int)i; break; }
        }
        for (auto& [pid, player] : players) {
            player_next_checkpoint[pid] = first_idx;
        }
    }
    
    if (spawn_points.empty()) {
        std::cerr << "[GameLoop]   NO HAY SPAWN POINTS! Usando posiciones por defecto" << std::endl;
    }
    
    std::cout << "[GameLoop] >>> Reseteando jugadores con Box2D..." << std::endl;
    
    size_t idx = 0;
    for (auto& [id, player] : players) {
        if (!player->getCar()) {
            std::cout << "[GameLoop]   Jugador " << id << " no tiene auto, saltando..." << std::endl;
            continue;
        }
        
        player->resetForNewRace();
        player->getCar()->reset();
        
        float x = 100.f, y = 100.f, a = 0.f;
        if (idx < spawn_points.size()) {
            std::tie(x, y, a) = spawn_points[idx];
        }
        
        // NUEVO: Crear cuerpo físico en Box2D (pasamos puntero al world_id)
        if (physics_world_created) {
            player->getCar()->createPhysicsBody(&physics_world_id, x, y, a);
            std::cout << "[GameLoop]   Physics body created for player " << id << std::endl;
        } else {
            std::cerr << "[GameLoop]   WARNING: Physics world not created, using legacy positioning" << std::endl;
            // Fallback: posicionamiento legacy
            player->setPosition(x, y);
            player->setAngle(a);
            player->getCar()->setPosition(x, y);
            player->getCar()->setAngle(a);
        }
        
        player_prev_pos[id] = {x, y};
        
        idx++;
    }
    
    std::cout << "[GameLoop] <<< Reseteo completado con " << idx << " jugadores" << std::endl;
}

void GameLoop::start_current_race() {
    if (current_race_index < races.size()) {
        current_map_yaml = races[current_race_index]->get_map_path();
        current_city_name = races[current_race_index]->get_city_name();
        current_race_finished = false;
        race_start_time = std::chrono::steady_clock::now();
    }
}

void GameLoop::finish_current_race() {
    std::cout << "[GameLoop] Carrera terminada.\n";
    print_current_race_table();
    
    current_race_index++;
    if (current_race_index >= races.size()) {
        match_finished = true;
        print_total_standings();
    } else {
        // Pausa entre carreras
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Preparar la siguiente (resetear posiciones)
        if (current_race_index < races.size()) {
            const auto& next_race = races[current_race_index];
            current_map_yaml = next_race->get_map_path();
            current_city_name = next_race->get_city_name();
            current_race_finished = false;
            
            reset_players_for_race();
            race_start_time = std::chrono::steady_clock::now();
        }
    }
}

bool GameLoop::all_players_finished_race() const {
    for (const auto& [id, p] : players) {
        if (!p->isFinished() && !p->isDisconnected()) return false;
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
    if (current_race_index < race_finish_times.size()) {
        race_finish_times[current_race_index][player_id] = finish_time_ms;
    }

    // Actualizar tiempo total
    total_times[player_id] += finish_time_ms;

    std::cout << "[GameLoop] " << player->getName() << " terminó la carrera #"
              << (current_race_index + 1) << " en " << (finish_time_ms / 1000.0f) << "s\n";

    print_current_race_table();
}

void GameLoop::mark_player_finished_with_time(int player_id, uint32_t finish_time_ms) {
    auto it = players.find(player_id);
    if (it == players.end()) return;
    Player* player = it->second.get();
    if (player->isFinished()) return;

    player->markAsFinished();

    if (current_race_index < race_finish_times.size()) {
        race_finish_times[current_race_index][player_id] = finish_time_ms;
    }
    total_times[player_id] += finish_time_ms;

    std::cout << "[GameLoop] " << player->getName() << " terminó la carrera #"
              << (current_race_index + 1) << " en " << (finish_time_ms / 1000.0f) << "s\n";
}

void GameLoop::print_current_race_table() const {
    std::cout << "\n--- RESULTADOS CARRERA " << (current_race_index + 1) << " ---\n";
    if (current_race_index < race_finish_times.size()) {
        const auto& times = race_finish_times[current_race_index];
        for (const auto& [pid, time] : times) {
            std::cout << "Player " << pid << ": " << (time/1000.0f) << "s\n";
        }
    }
}

void GameLoop::print_total_standings() const {
    std::cout << "\n--- TABLA GENERAL ---\n";
    for (const auto& [pid, total] : total_times) {
        std::cout << "Player " << pid << ": " << (total/1000.0f) << "s\n";
    }
}

void GameLoop::print_match_info() const {
    std::cout << "Match info: " << players.size() << " players, " << races.size() << " races.\n";
}
