#include "match.h"

#include <arpa/inet.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <algorithm> 
#include <cctype>    
#include <thread>
#include <chrono>

#include "../../common_src/config.h"
#include "../../common_src/dtos.h"

// ============================================
// HELPERS
// ============================================

static std::string normalize_map_name(std::string city_name) {
    std::string filename = city_name;
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return std::tolower(c); });
    std::replace(filename.begin(), filename.end(), ' ', '-');
    return filename + ".yaml";
}

static std::string normalize_race_filename(std::string race_name) {
    std::string filename = race_name;
    std::replace(filename.begin(), filename.end(), ' ', '_');
    if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".yaml") {
        filename += ".yaml";
    }
    return filename;
}

// ============================================
// CONSTRUCTOR & DESTRUCTOR
// ============================================

Match::Match(std::string host_name, int code, int max_players)
    : host_name(std::move(host_name)), 
      match_code(code), 
      is_active(false),
      state(MatchState::WAITING),
      current_race_index(0),
      game_thread(nullptr),  // <-- Inicializar game_thread como nullptr
      command_queue(),
      max_players(max_players),
      broadcast_callback(nullptr) {
    
    std::cout << "[Match] Creado: " << this->host_name << " (code=" << code << ", max=" << max_players << ")\n";
    
    // Inicializamos el GameLoop (pero NO creamos el thread todavía)
    gameloop = std::make_unique<GameLoop>(command_queue, players_queues);
}

Match::~Match() {
    stop_match();
    
    // Esperar a que el thread del GameLoop termine
    if (game_thread && game_thread->joinable()) {
        game_thread->join();
    }
}

// ============================================
// START MATCH (MANUAL)
// ============================================

void Match::start_match() {
    std::cout << "[Match] ════════════════════════════════════════════\n";
    std::cout << "[Match] 🏁 INICIANDO PARTIDA CODE " << match_code << "\n";
    std::cout << "[Match] ════════════════════════════════════════════\n";

    std::lock_guard<std::mutex> lock(mtx);
    
    if (state == MatchState::STARTED) {
        std::cout << "[Match] Ya está iniciada\n";
        return;
    }

    // Validación manual de seguridad
    bool all_ready = true;
    if (players_info.empty()) all_ready = false;
    for (const auto& [id, info] : players_info) {
        if (!info.is_ready) { 
            all_ready = false; 
            break; 
        }
    }

    if (!all_ready || race_configs.empty()) {
        std::cout << "[Match] No se puede iniciar (Faltan jugadores listos o carreras)\n";
        return;
    }

    // 1. Cambiar estado
    state = MatchState::STARTED;
    is_active.store(true);

    if (gameloop) {
        gameloop->begin_match();  // <-- Pone start_game_signal = true
    }

    // 2. Crear y lanzar el thread del GameLoop (si no existe ya)
    if (gameloop && !game_thread) {
        game_thread = std::make_unique<std::thread>([this]() {
            gameloop->run();
        });
        std::cout << "[Match] Thread de GameLoop creado y lanzado\n";
    }
    
    std::cout << "[Match] Partida en marcha (GameLoop ejecutándose en thread separado).\n";
}

void Match::stop_match() {
    is_active.store(false);
    if (gameloop) {
        gameloop->stop_match();
    }
}

// ============================================
// GESTIÓN DE JUGADORES
// ============================================

bool Match::can_player_join_match() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return static_cast<int>(players_info.size()) < max_players && state != MatchState::STARTED;
}

bool Match::add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora) {
    std::lock_guard<std::mutex> lock(mtx);

    if (static_cast<int>(players_info.size()) >= max_players || state == MatchState::STARTED) {
        return false;
    }

    players_queues.add_client_queue(queue_enviadora, id);

    PlayerLobbyInfo info;
    info.id = id;
    info.name = nombre;
    info.is_ready = false;
    info.sender_queue = &queue_enviadora;

    players_info[id] = info;
    player_name_to_id[nombre] = id;

    if (static_cast<int>(players_info.size()) >= 2) {
        state = MatchState::READY;
    }

    std::cout << "[Match] Jugador agregado: " << nombre << " (id=" << id << ", total=" << players_info.size() << ")\n";
    print_players_info();
    return true;
}

bool Match::remove_player(int id_jugador) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = players_info.find(id_jugador);
    if (it == players_info.end()) return false;

    std::string nombre = it->second.name;

    players_info.erase(it);
    player_name_to_id.erase(nombre);
    players_queues.delete_client_queue(id_jugador);

    if (static_cast<int>(players_info.size()) < 2) {
        state = MatchState::WAITING;
    }

    std::cout << "[Match] Jugador eliminado: " << nombre << " (id=" << id_jugador << ", restantes=" << players_info.size() << ")\n";
    return true;
}

// Getters con bloqueo
bool Match::has_player(int player_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info.find(player_id) != players_info.end();
}

bool Match::has_player_by_name(const std::string& name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return player_name_to_id.find(name) != player_name_to_id.end();
}

int Match::get_player_id_by_name(const std::string& name) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = player_name_to_id.find(name);
    return (it != player_name_to_id.end()) ? it->second : -1;
}

bool Match::is_empty() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info.empty();
}

int Match::get_player_count() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info.size();
}

std::map<int, PlayerLobbyInfo> Match::get_players_snapshot() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info;
}

const std::map<int, PlayerLobbyInfo>& Match::get_players() const { 
    return players_info; 
}

// ============================================
// LÓGICA DE PREPARACIÓN (READY / CARS)
// ============================================

bool Match::set_player_car(int player_id, const std::string& car_name, const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    it->second.car_name = car_name;
    it->second.car_type = car_type;

    std::cout << "[Match] Jugador " << it->second.name << " eligió auto " << car_name << "\n";

    if (gameloop) {
        gameloop->add_player(player_id, it->second.name, car_name, car_type);
    }

    print_players_info();
    return true;
}

bool Match::set_player_ready(int player_id, bool ready) {
    std::lock_guard<std::mutex> lock(mtx);
    
    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] No car selected for " << it->second.name << "\n";
        return false;
    }

    it->second.is_ready = ready;

    if (gameloop) {
        gameloop->set_player_ready(player_id, ready);
    }
    
    print_players_info();
    
    // NOTA: Aquí NO hay auto-start. El servidor espera MSG_START_GAME del host.
    return true;
}

// Helpers de configuración
bool Match::set_player_ready_by_name(const std::string& player_name, bool ready) {
    int id = get_player_id_by_name(player_name);
    return (id != -1) ? set_player_ready(id, ready) : false;
}

bool Match::set_player_car_by_name(const std::string& p, const std::string& c, const std::string& t) {
    int id = get_player_id_by_name(p);
    return (id != -1) ? set_player_car(id, c, t) : false;
}

std::string Match::get_player_car(int player_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = players_info.find(player_id);
    return (it != players_info.end()) ? it->second.car_name : "";
}

bool Match::all_players_selected_car() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    for (const auto& [id, info] : players_info) {
        if (info.car_name.empty()) return false;
    }
    return !players_info.empty();
}

bool Match::is_player_ready(int player_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = players_info.find(player_id);
    return (it != players_info.end()) ? it->second.is_ready : false;
}

bool Match::all_players_ready() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    if (players_info.empty()) return false;
    for (const auto& [id, info] : players_info) {
        if (!info.is_ready) return false;
    }
    return true;
}

bool Match::can_start() const {
    return state == MatchState::READY && all_players_ready() && !race_configs.empty();
}


// ============================================
// CONFIGURACIÓN DE CARRERAS
// ============================================

void Match::set_race_configs(const std::vector<ServerRaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs;
    races.clear(); 

    for (const auto& config : configs) {
        std::string base = normalize_map_name(config.city);
        std::string path = "server_src/city_maps/" + config.city + "/" + base;
        std::string race = "server_src/city_maps/" + config.city + "/" + normalize_race_filename(config.race_name);
        add_race(path, race, config.city);
    }

    if (gameloop) {
        gameloop->set_races(std::move(races));
    }
    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras.\n";
}

void Match::add_race(const std::string& map_path, const std::string& race_path, const std::string& city_name) {
    races.push_back(std::make_unique<Race>(city_name, map_path, race_path));
    std::cout << "[Match] + Carrera preparada: " << city_name << "\n";
}

std::vector<std::string> Match::get_race_yaml_paths() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    std::vector<std::string> paths;
    for (const auto& config : race_configs) {
        std::string race = normalize_race_filename(config.race_name);
        paths.push_back("server_src/city_maps/" + config.city + "/" + race);
    }
    return paths;
}

// ============================================
// MENSAJES / BROADCAST (Helpers)
// ============================================

void Match::send_race_info_to_all_players() {
    // Si se necesita enviar info extra a través del callback del monitor
    if (race_configs.empty() || !broadcast_callback) return;
    
    // (Lógica de serialización si fuera necesaria...)
    // En la versión actual, el Receiver ya maneja el broadcast de inicio.
}

void Match::send_game_started_confirmation() {
    if (!broadcast_callback) return;
    std::vector<uint8_t> buffer = { static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED) };
    broadcast_callback(buffer, -1);
}

void Match::print_players_info() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MATCH #" << match_code << " - JUGADORES (" << players_info.size() << "/"
              << max_players << ")";
    std::cout << std::string(26, ' ') << "║\n";
    std::cout << "╠════════════════════════════════════════════════════════════╣\n";

    if (players_info.empty()) {
        std::cout << "║  [VACÍO] No hay jugadores en esta partida" << std::string(17, ' ') << "║\n";
    } else {
        for (const auto& [player_id, info] : players_info) {
            std::cout << "║ ID: " << std::setw(3) << player_id << " │ ";
            std::cout << std::setw(15) << std::left << info.name << " │ ";

            if (info.car_name.empty()) {
                std::cout << std::setw(20) << "[SIN AUTO]";
            } else {
                std::string car_info = info.car_name + " (" + info.car_type + ")";
                std::cout << std::setw(20) << car_info.substr(0, 20);
            }

            std::cout << " │ " << (info.is_ready ? "✓ READY" : "✗ NOT READY");
            std::cout << " ║\n";
        }
    }
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
}