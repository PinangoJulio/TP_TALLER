#include "match.h"

#include <arpa/inet.h>
#include <cstring>
#include <iomanip>
#include <utility>
#include <vector> 
#include <iostream>
#include <algorithm> // transform
#include <cctype>    // tolower

#include "../../common_src/config.h"
#include "../../common_src/dtos.h"

// ============================================
// HELPER: Normalizar nombres de archivo
// ============================================

static std::string normalize_map_name(std::string city_name) {
    std::string filename = city_name;
    std::transform(filename.begin(), filename.end(), filename.begin(),
                   [](unsigned char c){ return std::tolower(c); });
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
// CONSTRUCTOR
// ============================================

Match::Match(std::string host_name, int code, int max_players)
    : host_name(std::move(host_name)), match_code(code), is_active(false),
      state(MatchState::WAITING), current_race_index(0), players_queues(), command_queue(),
      max_players(max_players) {
    
    std::cout << "[Match] Creado: " << this->host_name << " (code=" << code
              << ", max_players=" << max_players << ")\n";
              
    // ✅ INICIALIZAR GAMELOOP ÚNICO
    // Le pasamos las colas para que pueda escuchar comandos y enviar broadcasts
    gameloop = std::make_unique<GameLoop>(command_queue, players_queues);
    
    // Arrancamos el thread del GameLoop (se quedará esperando carreras)
    gameloop->start();
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

    std::cout << "[Match] Jugador agregado: " << nombre << " (id=" << id
              << ", total=" << players_info.size() << ")\n";
    
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

    std::cout << "[Match] Jugador eliminado: " << nombre << " (id=" << id_jugador
              << ", restantes=" << players_info.size() << ")" << std::endl;

    return true;
}

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

// ============================================
// SELECCIÓN DE AUTO
// ============================================

bool Match::set_player_car(int player_id, const std::string& car_name, const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    it->second.car_name = car_name;
    it->second.car_type = car_type;

    std::cout << "[Match] Jugador " << it->second.name << " eligió auto " << car_name << "\n";

    // ✅ CORRECCIÓN: Delegar al GameLoop directamente, no iterar 'races'
    if (gameloop) {
        gameloop->add_player(player_id, it->second.name, car_name, car_type);
    }

    print_players_info();
    return true;
}

bool Match::set_player_car_by_name(const std::string& player_name, const std::string& car_name,
                                   const std::string& car_type) {
    int player_id = get_player_id_by_name(player_name);
    if (player_id == -1) return false;
    return set_player_car(player_id, car_name, car_type);
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

// ============================================
// ESTADO READY
// ============================================

bool Match::set_player_ready(int player_id, bool ready) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] No car selected for " << it->second.name << "\n";
        return false;
    }

    it->second.is_ready = ready;

    // ✅ CORRECCIÓN: Delegar al GameLoop directamente
    if (gameloop) {
        gameloop->set_player_ready(player_id, ready);
    }
    
    print_players_info();
    return true;
}

bool Match::set_player_ready_by_name(const std::string& player_name, bool ready) {
    int player_id = get_player_id_by_name(player_name);
    if (player_id == -1) return false;
    return set_player_ready(player_id, ready);
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

std::map<int, PlayerLobbyInfo> Match::get_players_snapshot() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info;
}

const std::map<int, PlayerLobbyInfo>& Match::get_players() const {
    return players_info;
}

// ============================================
// CARRERAS 
// ============================================

void Match::set_race_configs(const std::vector<ServerRaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs;
    races.clear(); // Limpiamos el vector temporal de Match

    for (const auto& config : configs) {
        // 1. Mapa Base
        std::string base_filename = normalize_map_name(config.city);
        std::string base_map_path = "server_src/city_maps/" + config.city + "/" + base_filename;
        
        // 2. Configuración de Carrera
        std::string race_filename = normalize_race_filename(config.race_name);
        std::string race_config_path = "server_src/city_maps/" + config.city + "/" + race_filename;
        
        // Llamamos a add_race (que ahora SÍ usa los parámetros)
        add_race(base_map_path, race_config_path, config.city);
    }

    // IMPORTANTE: Transferimos la propiedad de las carreras al GameLoop
    if (gameloop) {
        gameloop->set_races(std::move(races));
    }

    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras y transferidas al GameLoop.\n";
}

void Match::add_race(const std::string& map_path, const std::string& race_path, const std::string& city_name) {
    // Usamos el constructor nuevo de Race (DTO)
    auto new_race = std::make_unique<Race>(city_name, map_path, race_path);

    // Lo guardamos en el vector temporal de Match
    races.push_back(std::move(new_race));
    
    // Usamos los parámetros en el cout para evitar el error "unused parameter"
    std::cout << "[Match]  Carrera preparada: " << city_name << "\n"
              << "        Base: " << map_path << "\n"
              << "        Config: " << race_path << "\n";
}

// ============================================
// ENVÍO DE INFO
// ============================================

void Match::send_race_info_to_all_players() {
    if (race_configs.empty()) return;

    const auto& first_config = race_configs[0];
    
    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));

    std::strncpy(race_info.city_name, first_config.city.c_str(), sizeof(race_info.city_name) - 1);
    std::strncpy(race_info.race_name, first_config.race_name.c_str(), sizeof(race_info.race_name) - 1);
    
    std::string base_filename = normalize_map_name(first_config.city);
    std::string map_path = "server_src/city_maps/" + first_config.city + "/" + base_filename;
    std::strncpy(race_info.map_file_path, map_path.c_str(), sizeof(race_info.map_file_path) - 1);

    race_info.total_laps = 3; 
    race_info.race_number = 1;
    race_info.total_races = static_cast<uint8_t>(race_configs.size());
    race_info.total_checkpoints = 0; 
    race_info.max_time_ms = 600000;

    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::RACE_INFO));

    auto push_string = [&](const std::string& s) {
        uint16_t len = htons(s.size());
        buffer.push_back(reinterpret_cast<uint8_t*>(&len)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&len)[1]);
        buffer.insert(buffer.end(), s.begin(), s.end());
    };

    push_string(race_info.city_name);
    push_string(race_info.race_name);
    push_string(race_info.map_file_path);

    buffer.push_back(race_info.total_laps);
    buffer.push_back(race_info.race_number);
    buffer.push_back(race_info.total_races);
    
    uint16_t cps = htons(race_info.total_checkpoints);
    buffer.push_back(reinterpret_cast<uint8_t*>(&cps)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&cps)[1]);

    uint32_t time = htonl(race_info.max_time_ms);
    buffer.push_back(reinterpret_cast<uint8_t*>(&time)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&time)[1]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&time)[2]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&time)[3]);

    if (broadcast_callback) {
        broadcast_callback(buffer, -1);
    }
    
    std::cout << "[Match] ✅ Información de carrera enviada a " << players_info.size() << " jugadores\n";
}

void Match::send_game_started_confirmation() {
    std::cout << "[Match] Enviando MSG_GAME_STARTED a todos los jugadores..." << std::endl;

    if (!broadcast_callback) {
        std::cerr << "[Match] ❌ No hay broadcast_callback configurado" << std::endl;
        return;
    }

    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED));

    broadcast_callback(buffer, -1);
    std::cout << "[Match] ✅ MSG_GAME_STARTED enviado.\n";
}

// ============================================
// INICIO
// ============================================

void Match::start_match() {
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
    std::cout << "[Match] 🏁 INICIANDO PARTIDA CODE " << match_code << std::endl;
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;

    std::lock_guard<std::mutex> lock(mtx);
    if (state == MatchState::STARTED) {
        std::cout << "[Match] Ya está iniciada" << std::endl;
        return;
    }

    if (!can_start()) {
        std::cout << "[Match] No se puede iniciar (no todos listos o sin carreras)" << std::endl;
        return;
    }

    state = MatchState::STARTED;
    is_active.store(true);

    // 1. Enviar confirmación
    send_game_started_confirmation();
    
    // 2. Enviar datos de la primera carrera
    send_race_info_to_all_players();
    
    // 3. El GameLoop ya está corriendo en su thread (iniciado en constructor).
    // Al haberle pasado las 'races' en set_race_configs, el GameLoop detectará 
    // que ya no está vacío y comenzará la secuencia automáticamente.
    
    std::cout << "[Match] Partida en marcha.\n";
}

// Ya no necesitamos start_next_race() porque el GameLoop maneja el bucle.
// Dejamos una implementación vacía o la borramos de match.h
void Match::start_next_race() {
    // Delegado al GameLoop
}

void Match::stop_match() {
    std::cout << "[Match] Deteniendo partida...\n";
    is_active.store(false);
    if (gameloop) {
        gameloop->stop_match();
    }
}

Match::~Match() {
    stop_match();
    if (gameloop) {
        gameloop->join();
    }
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