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
      spawns_loaded(false)
{
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
            if (checkpoint_debug_enabled) {
                std::cout << "[Checkpoint] player=" << pid << " crossed id=" << cp.id << " type=" << cp.type << " pos=(" << player_ptr->getX() << "," << player_ptr->getY() << ")\n";
            }
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
                if (checkpoint_debug_enabled) {
                    std::cout << "[Checkpoint] player=" << pid << " skip-> id=" << cp_k.id << " type=" << cp_k.type << " pos=(" << player_ptr->getX() << "," << player_ptr->getY() << ")\n";
                }
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

            // ✅ detección de checkpoints
            update_checkpoints();

            if (all_players_finished_race()) {
                current_race_finished = true;
            }

            enviar_estado_a_jugadores();

            // ✅ actualizar posición previa por jugador
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

// REEMPLAZAR procesar_comandos() completo en game_loop.cpp

// void GameLoop::procesar_comandos() {
//     ComandMatchDTO comando;
//     float delta_time = SLEEP / 1000.0f;

//     while (comandos.try_pop(comando)) {
//         auto it = players.find(comando.player_id);
//         if (it == players.end()) continue;

//         Player* player = it->second.get();
//         Car* car = player->getCar();
//         if (!car) continue;

//         // ✅ Guardar posición y nivel anteriores antes de aplicar comando
//         float prev_x = car->getX();
//         float prev_y = car->getY();
//         int player_level = player->getLevel();

//         // Aplicar el comando (modifica posición del auto)
//         switch (comando.command) {
//             case GameCommand::ACCELERATE: 
//                 car->accelerate(delta_time * comando.speed_boost); 
//                 break;
//             case GameCommand::BRAKE: 
//                 car->brake(delta_time * comando.speed_boost); 
//                 break;
//             case GameCommand::TURN_LEFT: 
//                 car->turn_left(delta_time * comando.turn_intensity); 
//                 break;
//             case GameCommand::TURN_RIGHT: 
//                 car->turn_right(delta_time * comando.turn_intensity); 
//                 break;

//             // Movimiento en 4 direcciones fijas
//             case GameCommand::MOVE_UP: 
//                 car->move_up(delta_time); 
//                 break;
//             case GameCommand::MOVE_DOWN: 
//                 car->move_down(delta_time); 
//                 break;
//             case GameCommand::MOVE_LEFT: 
//                 car->move_left(delta_time); 
//                 break;
//             case GameCommand::MOVE_RIGHT: 
//                 car->move_right(delta_time); 
//                 break;

//             case GameCommand::USE_NITRO: 
//                 car->activateNitro(); 
//                 break;
//             case GameCommand::DISCONNECT: 
//                 player->disconnect(); 
//                 break;
//             case GameCommand::STOP_ALL: 
//                 car->setCurrentSpeed(0); 
//                 car->setVelocity(0,0); 
//                 break;
                
//             // Cheats
//             case GameCommand::CHEAT_INVINCIBLE: 
//                 car->repair(1000.0f); 
//                 break;
//             case GameCommand::CHEAT_WIN_RACE: 
//                 player->markAsFinished(); 
//                 break;
//             case GameCommand::CHEAT_MAX_SPEED: 
//                 car->setCurrentSpeed(car->getMaxSpeed()); 
//                 break;
//             default: 
//                 break;
//         }
        
//         // ✅ VALIDACIÓN DE COLISIÓN: Verificar nueva posición después del comando
//         if (collision_manager) {
//             float new_x = car->getX();
//             float new_y = car->getY();
            
//             // can_move_to() valida colisiones y actualiza player_level si hay transición
//             if (!can_move_to(prev_x, prev_y, new_x, new_y, player_level)) {
//                 // Movimiento inválido - revertir posición
//                 car->setPosition(prev_x, prev_y);
//                 car->setCurrentSpeed(0.0f);
//                 car->setVelocity(0.0f, 0.0f);
                
//                 // Debug opcional
//                 // std::cout << "[GameLoop] 🚫 Colisión detectada para jugador " 
//                 //           << comando.player_id << " en (" << new_x << ", " << new_y << ")" 
//                 //           << std::endl;
//             } else {
//                 // Movimiento válido - actualizar nivel del jugador si cambió
//                 player->setLevel(player_level);
//             }
//         }
//     }
// }

// void GameLoop::procesar_comandos() {
//     ComandMatchDTO comando;
//     float delta_time = SLEEP / 1000.0f;

//     while (comandos.try_pop(comando)) {
//         auto it = players.find(comando.player_id);
//         if (it == players.end()) continue;

//         Player* player = it->second.get();
//         Car* car = player->getCar();
//         if (!car) continue;

//         switch (comando.command) {
//             case GameCommand::ACCELERATE: car->accelerate(delta_time * comando.speed_boost); break;
//             case GameCommand::BRAKE: car->brake(delta_time * comando.speed_boost); break;
//             case GameCommand::TURN_LEFT: car->turn_left(delta_time * comando.turn_intensity); break;
//             case GameCommand::TURN_RIGHT: car->turn_right(delta_time * comando.turn_intensity); break;

//             // Movimiento en 4 direcciones fijas
//             case GameCommand::MOVE_UP: car->move_up(delta_time); break;
//             case GameCommand::MOVE_DOWN: car->move_down(delta_time); break;
//             case GameCommand::MOVE_LEFT: car->move_left(delta_time); break;
//             case GameCommand::MOVE_RIGHT: car->move_right(delta_time); break;

//             case GameCommand::USE_NITRO: car->activateNitro(); break;
//             case GameCommand::DISCONNECT: player->disconnect(); break;
//             case GameCommand::STOP_ALL: car->setCurrentSpeed(0); car->setVelocity(0,0); break;
//             // Cheats
//             case GameCommand::CHEAT_INVINCIBLE: car->repair(1000.0f); break;
//             case GameCommand::CHEAT_WIN_RACE: player->markAsFinished(); break;
//             case GameCommand::CHEAT_MAX_SPEED: car->setCurrentSpeed(car->getMaxSpeed()); break;
//             default: break;
//         }
//     }
// }

void GameLoop::actualizar_fisica() { 
    // Integración Box2D
    // for (auto& [id, player] : players) { if (player->getCar()) player->getCar()->update(SLEEP/1000.0f); }
}
//void GameLoop::detectar_colisiones() { /* Collision logic here */ }

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
                std::cout << "[GameLoop]   Spawn point: (" << x << ", " << y << ") angle=" << a << " rad" << std::endl;
            }
            std::cout << "[GameLoop]  Cargados " << spawn_points.size() << " spawn points" << std::endl;
        } else {
            std::cout << "[GameLoop]  No se encontraron spawn points en el YAML." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop]  Error cargando spawns de " << current_map_yaml << ": " << e.what() << std::endl;
    }
}

// void GameLoop::reset_players_for_race() {
//     std::cout << "[GameLoop] >>> RESETEANDO JUGADORES PARA NUEVA CARRERA" << std::endl;
//     std::cout << "[GameLoop]   Mapa actual: " << current_map_yaml << std::endl;
//     load_spawn_points_for_current_race();
//     load_checkpoints_for_current_race();
//     try {
//         YAML::Node cfg = YAML::LoadFile("config.yaml");
//         if (cfg["checkpoint_tolerance_base"]) checkpoint_tol_base = cfg["checkpoint_tolerance_base"].as<float>();
//         if (cfg["checkpoint_tolerance_finish"]) checkpoint_tol_finish = cfg["checkpoint_tolerance_finish"].as<float>();
//         if (cfg["checkpoint_lookahead"]) checkpoint_lookahead = cfg["checkpoint_lookahead"].as<int>();
//         if (cfg["checkpoint_debug_enabled"]) checkpoint_debug_enabled = cfg["checkpoint_debug_enabled"].as<bool>();
//     } catch (...) {}
//     player_next_checkpoint.clear();
//     player_prev_pos.clear();
//     if (!checkpoints.empty()) {
//         int first_idx = 0;
//         for (size_t i = 0; i < checkpoints.size(); ++i) {
//             if (checkpoints[i].type != "start") { first_idx = (int)i; break; }
//         }
//         for (auto& [pid, player] : players) {
//             player_next_checkpoint[pid] = first_idx;
//         }
//     }
//     if (spawn_points.empty()) {
//         std::cerr << "[GameLoop]   NO HAY SPAWN POINTS! Usando posiciones por defecto" << std::endl;
//     }

//     size_t idx = 0;
//     for (auto& [id, player] : players) {
//         if (!player->getCar()) {
//             std::cout << "[GameLoop]   Jugador " << id << " no tiene auto, saltando..." << std::endl;
//             continue;
//         }

//         player->resetForNewRace();
//         player->getCar()->reset();

//         float x = 100.f, y = 100.f, a = 0.f;
//         if (idx < spawn_points.size()) {
//             std::tie(x, y, a) = spawn_points[idx];
//         }

//         player->setPosition(x, y);
//         player->setAngle(a);
//         player->getCar()->setPosition(x, y);
//         player->getCar()->setAngle(a);
//         player_prev_pos[id] = {x, y};

//         idx++;
//     }
//     std::cout << "[GameLoop] <<< Reseteo completado" << std::endl;
// }

void GameLoop::start_current_race() {
    if (current_race_index < races.size()) {
        current_map_yaml = races[current_race_index]->get_map_path();
        current_city_name = races[current_race_index]->get_city_name();
        current_race_finished = false;
        race_start_time = std::chrono::steady_clock::now();
        std::cout << "[GameLoop] Iniciando carrera: " << current_city_name << "\n";
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

// ✅ IMPLEMENTACIÓN COMPLETA PARA EVITAR WARNINGS
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


// ============================================================================
// MÉTODOS NUEVOS PARA COLISIONES (agregar a game_loop.cpp)
// ============================================================================

// Cargar el CollisionManager cuando se inicia una carrera
void GameLoop::load_collision_manager_for_current_race() {
    try {
        YAML::Node config = YAML::LoadFile(current_map_yaml);
        
        if (!config["race"] || !config["race"]["city"]) {
            throw std::runtime_error("YAML sin campo 'race.city'");
        }

        std::string raw_city = config["race"]["city"].as<std::string>();
        
        // Normalizar nombre de ciudad
        auto normalize = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c){ return std::tolower(c); });
            std::replace(s.begin(), s.end(), '_', '-');
            std::replace(s.begin(), s.end(), ' ', '-');
            return s;
        };
        
        std::string city_name = normalize(raw_city);
        
        // Rutas a las capas de colisión
        std::string layer_root = "assets/img/map/layers/" + city_name + "/";
        std::string collision_file = layer_root + "camino.png";
        std::string bridges_mask   = layer_root + "puentes.png";
        std::string ramps_file     = layer_root + "rampas.png";
        
        std::cout << "[GameLoop] 🗺️  Cargando collision manager..." << std::endl;
        std::cout << "[GameLoop]   Camino: " << collision_file << std::endl;
        std::cout << "[GameLoop]   Puentes: " << bridges_mask << std::endl;
        std::cout << "[GameLoop]   Rampas: " << ramps_file << std::endl;
        
        // Crear el collision manager
        collision_manager = std::make_unique<CollisionManager>(
            collision_file, 
            bridges_mask, 
            ramps_file
        );
        
        current_map_width = collision_manager->GetWidth();
        current_map_height = collision_manager->GetHeight();
        
        std::cout << "[GameLoop] ✅ Collision manager cargado: " 
                  << current_map_width << "x" << current_map_height << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[GameLoop] ❌ Error cargando collision manager: " 
                  << e.what() << std::endl;
        collision_manager.reset();
    }
}

// Verificar si una posición es válida (no hay pared)
bool GameLoop::is_position_valid(float x, float y, int player_level) {
    if (!collision_manager) {
        return true; // Sin collision manager, permitir todo
    }
    
    // Verificar límites del mapa
    if (x < 0 || y < 0 || x >= current_map_width || y >= current_map_height) {
        return false;
    }
    
    // Verificar si hay pared en el nivel actual
    return !collision_manager->isWall(static_cast<int>(x), static_cast<int>(y), player_level);
}

// ✅ MÉTODO CORREGIDO para transiciones de nivel
bool GameLoop::can_move_to(float from_x, float from_y, float to_x, float to_y, int& player_level) {

    (void)from_x;
    (void)from_y;
    if (!collision_manager) {
        return true;
    }
    
    // Verificar límites básicos
    if (to_x < 0 || to_y < 0 || to_x >= current_map_width || to_y >= current_map_height) {
        return false;
    }
    
    bool next_has_ground = collision_manager->hasGroundLevel(to_x, to_y);
    bool next_has_bridge = collision_manager->hasBridgeLevel(to_x, to_y);
    bool next_is_ramp = collision_manager->isRamp(to_x, to_y);
    
    // 🔵 CASO 1: Estoy en CALLE (nivel 0)
    if (player_level == 0) {
        // Sub-caso 1.1: Siguiente posición tiene SOLO camino
        if (next_has_ground && !next_has_bridge) {
            return true; // Movimiento normal en calle
        }
        
        // Sub-caso 1.2: Siguiente posición tiene SOLO puente (sin camino)
        if (!next_has_ground && next_has_bridge) {
            // Necesito rampa para subir
            if (collision_manager->canTransition(to_x, to_y, 0, 1)) {
                player_level = 1; // ✅ Cambiar a nivel puente
                std::cout << "[GameLoop] 🚗 Jugador subiendo a puente en (" 
                          << to_x << ", " << to_y << ")" << std::endl;
                return true;
            }
            return false; // No puedo subir sin rampa
        }
        
        // Sub-caso 1.3: Siguiente posición es RAMPA (tiene ambos)
        if (next_is_ramp && next_has_ground && next_has_bridge) {
            // En la rampa puedo moverme libremente, sigo en nivel 0
            return true;
        }
        
        // Sin camino ni rampa válida
        return false;
    }
    
    // 🟢 CASO 2: Estoy en PUENTE (nivel 1)
    else if (player_level == 1) {
        // Sub-caso 2.1: Siguiente posición tiene SOLO puente
        if (next_has_bridge && !next_has_ground) {
            return true; // Movimiento normal en puente
        }
        
        // Sub-caso 2.2: Siguiente posición tiene SOLO camino (sin puente)
        if (next_has_ground && !next_has_bridge) {
            // Necesito rampa para bajar
            if (collision_manager->canTransition(to_x, to_y, 1, 0)) {
                player_level = 0; // ✅ Cambiar a nivel calle
                std::cout << "[GameLoop] 🚗 Jugador bajando a calle en (" 
                          << to_x << ", " << to_y << ")" << std::endl;
                return true;
            }
            return false; // No puedo bajar sin rampa
        }
        
        // Sub-caso 2.3: Siguiente posición es RAMPA (tiene ambos)
        if (next_is_ramp && next_has_bridge && next_has_ground) {
            // En la rampa puedo moverme libremente, sigo en nivel 1
            return true;
        }
        
        // Sin puente ni rampa válida
        return false;
    }
    
    // Nivel inválido
    return false;
}

// REEMPLAZAR el método detectar_colisiones() existente con este:
void GameLoop::detectar_colisiones() {
    if (!collision_manager) {
        return; // Sin collision manager, no validar
    }
    
    for (auto& [player_id, player_ptr] : players) {
        if (!player_ptr || !player_ptr->getCar()) continue;
        if (player_ptr->isFinished() || player_ptr->isDisconnected()) continue;
        
        Car* car = player_ptr->getCar();
        
        // Obtener posición actual y propuesta
        float current_x = car->getX();
        float current_y = car->getY();
        
        // Verificar si el auto está en una posición inválida
        // (esto puede pasar si spawneó mal o hubo un bug)
        // Necesitamos un nivel por jugador - agregar al Player si no existe
        int player_level = 0; // TODO: Debería venir de Player
        
        if (!is_position_valid(current_x, current_y, player_level)) {
            // Posición inválida - detener el auto
            car->setCurrentSpeed(0.0f);
            car->setVelocity(0.0f, 0.0f);
            
            std::cout << "[GameLoop] ⚠️ Jugador " << player_id 
                      << " en posición inválida (" << current_x << ", " << current_y 
                      << ") - frenado" << std::endl;
        }
    }
}
void GameLoop::procesar_comandos() {
    ComandMatchDTO comando;
    float delta_time = SLEEP / 1000.0f; // 0.016 segundos

    while (comandos.try_pop(comando)) {
        auto it = players.find(comando.player_id);
        if (it == players.end()) continue;

        Player* player = it->second.get();
        Car* car = player->getCar();
        if (!car) continue;

        // ✅ Guardar posición anterior
        float prev_x = car->getX();
        float prev_y = car->getY();
        int player_level = player->getLevel();

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

            // ✅ VELOCIDAD BALANCEADA: 5 píxeles por frame a 60 FPS = ~300 px/s
            // Ajustá este valor según tus necesidades (3.0f = lento, 8.0f = rápido)
            case GameCommand::MOVE_UP: 
                car->move_up(5.0f); 
                break;
            case GameCommand::MOVE_DOWN: 
                car->move_down(5.0f); 
                break;
            case GameCommand::MOVE_LEFT: 
                car->move_left(5.0f); 
                break;
            case GameCommand::MOVE_RIGHT: 
                car->move_right(5.0f); 
                break;

            case GameCommand::USE_NITRO: 
                car->activateNitro(); 
                break;
            case GameCommand::DISCONNECT: 
                player->disconnect(); 
                break;
            case GameCommand::STOP_ALL: 
                car->setCurrentSpeed(0); 
                car->setVelocity(0,0); 
                break;
                
            // Cheats
            case GameCommand::CHEAT_INVINCIBLE: 
                car->repair(1000.0f); 
                break;
            case GameCommand::CHEAT_WIN_RACE: 
                player->markAsFinished(); 
                break;
            case GameCommand::CHEAT_MAX_SPEED: 
                car->setCurrentSpeed(car->getMaxSpeed()); 
                break;
            default: 
                break;
        }
        
        // ✅ VALIDACIÓN: Verificar nueva posición después del comando
        if (collision_manager) {
            float new_x = car->getX();
            float new_y = car->getY();
            
            if (!can_move_to(prev_x, prev_y, new_x, new_y, player_level)) {
                car->setPosition(prev_x, prev_y);
                car->setCurrentSpeed(0.0f);
                car->setVelocity(0.0f, 0.0f);
            } else {
                player->setLevel(player_level);
            }
        }
    }
}

void GameLoop::reset_players_for_race() {
    std::cout << "[GameLoop] >>> RESETEANDO JUGADORES PARA NUEVA CARRERA" << std::endl;
    std::cout << "[GameLoop]   Mapa actual: " << current_map_yaml << std::endl;
    
    
    load_collision_manager_for_current_race();
    
    load_spawn_points_for_current_race();
    load_checkpoints_for_current_race();
    
    try {
        YAML::Node cfg = YAML::LoadFile("config.yaml");
        if (cfg["checkpoint_tolerance_base"]) 
            checkpoint_tol_base = cfg["checkpoint_tolerance_base"].as<float>();
        if (cfg["checkpoint_tolerance_finish"]) 
            checkpoint_tol_finish = cfg["checkpoint_tolerance_finish"].as<float>();
        if (cfg["checkpoint_lookahead"]) 
            checkpoint_lookahead = cfg["checkpoint_lookahead"].as<int>();
        if (cfg["checkpoint_debug_enabled"]) 
            checkpoint_debug_enabled = cfg["checkpoint_debug_enabled"].as<bool>();
    } catch (...) {}
    
    player_next_checkpoint.clear();
    player_prev_pos.clear();
    
    if (!checkpoints.empty()) {
        int first_idx = 0;
        for (size_t i = 0; i < checkpoints.size(); ++i) {
            if (checkpoints[i].type != "start") { 
                first_idx = (int)i; 
                break; 
            }
        }
        for (auto& [pid, player] : players) {
            player_next_checkpoint[pid] = first_idx;
        }
    }
    
    if (spawn_points.empty()) {
        std::cerr << "[GameLoop]   NO HAY SPAWN POINTS! Usando posiciones por defecto" << std::endl;
    }

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

        // ✅ VALIDAR que el spawn point sea válido
        if (collision_manager && !is_position_valid(x, y, 0)) {
            std::cerr << "[GameLoop] ⚠️ Spawn point inválido para jugador " << id 
                      << " en (" << x << ", " << y << ")" << std::endl;
            // Usar posición de emergencia
            x = 100.f;
            y = 100.f;
        }

        player->setPosition(x, y);
        player->setAngle(a);
        player->getCar()->setPosition(x, y);
        player->getCar()->setAngle(a);
        player_prev_pos[id] = {x, y};

        idx++;
    }
    std::cout << "[GameLoop] <<< Reseteo completado" << std::endl;
}