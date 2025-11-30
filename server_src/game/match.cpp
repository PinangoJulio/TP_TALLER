#include "match.h"

#include <arpa/inet.h>
#include <cstring>
#include <iomanip>
#include <utility>
#include <vector> 
#include <iostream>
#include <algorithm> // Necesario para transform
#include <cctype>    // Necesario para tolower

#include "../../common_src/config.h"
#include "../../common_src/dtos.h"

// ============================================
// HELPER: Normalizar nombre de archivo
// Convierte "Vice City" -> "vice-city.yaml"
// ============================================
static std::string normalize_map_name(std::string city_name) {
    std::string filename = city_name;
    
    // 1. Convertir a minúsculas
    std::transform(filename.begin(), filename.end(), filename.begin(),
                   [](unsigned char c){ return std::tolower(c); });
                   
    // 2. Reemplazar espacios y guiones bajos por guiones medios
    std::replace(filename.begin(), filename.end(), ' ', '-');
    std::replace(filename.begin(), filename.end(), '_', '-');
    
    return filename + ".yaml";
}

Match::Match(std::string host_name, int code, int max_players)
    : host_name(std::move(host_name)), match_code(code), is_active(false),
      state(MatchState::WAITING), current_race_index(0), players_queues(), command_queue(),
      max_players(max_players) {}

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

    if (!races.empty()) {
        for (auto& race : races) {
            race->add_player_to_gameloop(player_id, it->second.name, car_name, car_type);
        }
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

    if (!races.empty()) {
        for (auto& race : races) {
            race->set_player_ready(player_id, ready);
        }
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
// CARRERAS (CONFIGURACIÓN CORREGIDA)
// ============================================

void Match::set_race_configs(const std::vector<ServerRaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs;
    races.clear(); 

    for (const auto& config : configs) {
        // 1. MAPA BASE: Usamos la función para convertir "Vice City" -> "vice-city.yaml"
        std::string filename = normalize_map_name(config.city);
        std::string base_map_path = "server_src/city_maps/" + config.city + "/" + filename;
        
        // 2. CARRERA: Si la carrera no tiene extension .yaml, se la agregamos
        std::string race_config_path = "server_src/city_maps/" + config.city + "/" + config.race_name;
        if (race_config_path.substr(race_config_path.length() - 5) != ".yaml") {
            race_config_path += ".yaml";
        }

        add_race(base_map_path, race_config_path, config.city);
    }

    std::cout << "[Match] Configuradas " << races.size() << " carreras.\n";
}

void Match::add_race(const std::string& map_path, const std::string& race_path, const std::string& city_name) {
    auto new_race = std::make_unique<Race>(command_queue, players_queues, city_name, map_path, race_path);

    for (const auto& [id, info] : players_info) {
        if (!info.car_name.empty()) {
            new_race->add_player_to_gameloop(id, info.name, info.car_name, info.car_type);
        }
    }

    races.push_back(std::move(new_race));
    std::cout << "[Match] + Carrera: " << city_name << "\n"
              << "        - Base: " << map_path << "\n"
              << "        - Config: " << race_path << "\n";
}

// ============================================
// ENVÍO DE INFO
// ============================================

void Match::send_race_info_to_all_players() {
    if (races.empty() || race_configs.empty()) return;

    const auto& first_config = race_configs[0];
    
    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));

    std::strncpy(race_info.city_name, first_config.city.c_str(), sizeof(race_info.city_name) - 1);
    std::strncpy(race_info.race_name, first_config.race_name.c_str(), sizeof(race_info.race_name) - 1);
    
    // Construimos la ruta normalizada también para enviar al cliente
    std::string filename = normalize_map_name(first_config.city);
    std::string map_path = "server_src/city_maps/" + first_config.city + "/" + filename;
    
    std::strncpy(race_info.map_file_path, map_path.c_str(), sizeof(race_info.map_file_path) - 1);

    race_info.total_laps = 3; 
    race_info.race_number = 1;
    race_info.total_races = static_cast<uint8_t>(races.size());
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
}

// ============================================
// INICIO
// ============================================

void Match::start_match() {
    std::lock_guard<std::mutex> lock(mtx);
    if (state == MatchState::STARTED || !can_start()) return;

    state = MatchState::STARTED;
    send_race_info_to_all_players();
    start_next_race();
}

void Match::start_next_race() {
    if (races.empty() || current_race_index >= static_cast<int>(races.size())) {
        is_active.store(false);
        std::cout << "[Match] Fin de todas las carreras.\n";
        return;
    }

    is_active.store(true);
    auto& current_race = races[current_race_index];
    
    std::cout << "[Match] Iniciando carrera " << (current_race_index + 1) << "\n";
    
    current_race->set_total_laps(3); 
    current_race->start();
    
    current_race_index++;
}

Match::~Match() {
    is_active = false;
    races.clear();
}

void Match::print_players_info() const {
    std::cout << "--- Lista de Jugadores (" << players_info.size() << ") ---\n";
    for (const auto& [id, info] : players_info) {
        std::cout << "ID: " << id << " | " << info.name << " | " 
                  << (info.is_ready ? "READY" : "WAITING") << "\n";
    }
}