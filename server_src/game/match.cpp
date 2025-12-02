#include "match.h"
#include <arpa/inet.h>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <algorithm> 
#include <cctype>    
#include "../../common_src/config.h"
#include "../../common_src/dtos.h"

// Helpers
static std::string normalize_map_name(std::string city_name) {
    std::string filename = city_name;
    std::transform(filename.begin(), filename.end(), filename.begin(), [](unsigned char c){ return std::tolower(c); });
    std::replace(filename.begin(), filename.end(), ' ', '-');
    return filename + ".yaml";
}
static std::string normalize_race_filename(std::string race_name) {
    std::string filename = race_name;
    std::replace(filename.begin(), filename.end(), ' ', '_');
    if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".yaml") filename += ".yaml";
    return filename;
}

Match::Match(std::string host_name, int code, int max_players)
    : host_name(std::move(host_name)), match_code(code), is_active(false),
      state(MatchState::WAITING), current_race_index(0), players_queues(), command_queue(),
      max_players(max_players) {
    std::cout << "[Match] Creado: " << this->host_name << " (code=" << code << ", max=" << max_players << ")\n";
    gameloop = std::make_unique<GameLoop>(command_queue, players_queues);
    gameloop->start();
}

bool Match::can_player_join_match() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return static_cast<int>(players_info.size()) < max_players && state != MatchState::STARTED;
}

bool Match::add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora) {
    std::lock_guard<std::mutex> lock(mtx);
    if (static_cast<int>(players_info.size()) >= max_players || state == MatchState::STARTED) return false;

    players_queues.add_client_queue(queue_enviadora, id);
    PlayerLobbyInfo info;
    info.id = id;
    info.name = nombre;
    info.is_ready = false;
    info.sender_queue = &queue_enviadora;
    players_info[id] = info;
    player_name_to_id[nombre] = id;

    if (static_cast<int>(players_info.size()) >= 2) state = MatchState::READY;
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
    if (static_cast<int>(players_info.size()) < 2) state = MatchState::WAITING;
    std::cout << "[Match] Jugador eliminado: " << nombre << " (id=" << id_jugador << ", restantes=" << players_info.size() << ")\n";
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

bool Match::set_player_car(int player_id, const std::string& car_name, const std::string& car_type) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;
    it->second.car_name = car_name;
    it->second.car_type = car_type;
    std::cout << "[Match] Jugador " << it->second.name << " eligió auto " << car_name << "\n";
    if (gameloop) gameloop->add_player(player_id, it->second.name, car_name, car_type);
    print_players_info();
    return true;
}

// ============================================
// FIX DEADLOCK & AUTO-START
// ============================================

bool Match::set_player_ready(int player_id, bool ready) {
    std::lock_guard<std::mutex> lock(mtx); // Volvemos a lock_guard normal
    
    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] No car selected for " << it->second.name << "\n";
        return false;
    }

    it->second.is_ready = ready;
    if (gameloop) gameloop->set_player_ready(player_id, ready);
    
    print_players_info();

    return true;
}
//Para que la carrera inicie automatica
/*bool Match::set_player_ready(int player_id, bool ready) {
    std::unique_lock<std::mutex> lock(mtx); 
    
    auto it = players_info.find(player_id);
    if (it == players_info.end()) return false;

    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] No car selected for " << it->second.name << "\n";
        return false;
    }

    it->second.is_ready = ready;
    if (gameloop) gameloop->set_player_ready(player_id, ready);
    
    print_players_info();

    // AUTO-START LOGIC
    if (state != MatchState::STARTED && !race_configs.empty()) {
        bool all_ready = true;
        for (const auto& [id, info] : players_info) {
            if (!info.is_ready) {
                all_ready = false;
                break;
            }
        }
        
        if (all_ready && !players_info.empty()) {
             std::cout << "[Match] ⚡ TODOS LOS JUGADORES LISTOS. Iniciando partida automáticamente...\n";
             lock.unlock(); // DESBLOQUEAR ANTES DE LLAMAR A START_MATCH
             start_match();
             return true; 
        }
    }
    return true;
}*/

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
    for (const auto& [id, info] : players_info) if (info.car_name.empty()) return false;
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
    for (const auto& [id, info] : players_info) if (!info.is_ready) return false;
    return true;
}
bool Match::can_start() const {
    return state == MatchState::READY && all_players_ready() && !race_configs.empty();
}
std::map<int, PlayerLobbyInfo> Match::get_players_snapshot() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info;
}
const std::map<int, PlayerLobbyInfo>& Match::get_players() const { return players_info; }

// ============================================
// START MATCH
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

    // Verificación MANUAL sin llamar a can_start()
    bool all_ready = true;
    if (players_info.empty()) all_ready = false;
    for (const auto& [id, info] : players_info) {
        if (!info.is_ready) { all_ready = false; break; }
    }

    if (!all_ready || race_configs.empty()) {
        std::cout << "[Match] No se puede iniciar (Condiciones no cumplidas)\n";
        return;
    }

    state = MatchState::STARTED;
    is_active.store(true);

    send_game_started_confirmation();
    send_race_info_to_all_players();
    
    if (gameloop) gameloop->begin_match(); // Despertar gameloop
    
    std::cout << "[Match] Partida en marcha.\n";
}

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
    if (gameloop) gameloop->set_races(std::move(races));
    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras.\n";
}

void Match::add_race(const std::string& map_path, const std::string& race_path, const std::string& city_name) {
    races.push_back(std::make_unique<Race>(city_name, map_path, race_path));
    std::cout << "[Match] + Carrera preparada: " << city_name << "\n";
}

void Match::send_race_info_to_all_players() {
    if (race_configs.empty()) return;
    const auto& first_config = race_configs[0];
    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));
    std::strncpy(race_info.city_name, first_config.city.c_str(), sizeof(race_info.city_name) - 1);
    std::strncpy(race_info.race_name, first_config.race_name.c_str(), sizeof(race_info.race_name) - 1);
    std::string base = normalize_map_name(first_config.city);
    std::string map = "server_src/city_maps/" + first_config.city + "/" + base;
    std::strncpy(race_info.map_file_path, map.c_str(), sizeof(race_info.map_file_path) - 1);
    race_info.total_laps = 3; 
    race_info.race_number = 1;
    race_info.total_races = static_cast<uint8_t>(race_configs.size());
    race_info.total_checkpoints = 0; 
    race_info.max_time_ms = 600000;

    std::vector<uint8_t> buffer;
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::RACE_INFO));
    auto push_str = [&](const std::string& s) {
        uint16_t len = htons(s.size());
        buffer.push_back(((uint8_t*)&len)[0]); buffer.push_back(((uint8_t*)&len)[1]);
        buffer.insert(buffer.end(), s.begin(), s.end());
    };
    push_str(race_info.city_name); push_str(race_info.race_name); push_str(race_info.map_file_path);
    buffer.push_back(race_info.total_laps); buffer.push_back(race_info.race_number);
    buffer.push_back(race_info.total_races);
    uint16_t cps = htons(race_info.total_checkpoints);
    buffer.push_back(((uint8_t*)&cps)[0]); buffer.push_back(((uint8_t*)&cps)[1]);
    uint32_t time = htonl(race_info.max_time_ms);
    for(int i=0;i<4;i++) buffer.push_back(((uint8_t*)&time)[i]);

    if (broadcast_callback) broadcast_callback(buffer, -1);
}

void Match::send_game_started_confirmation() {
    std::cout << "[Match] Enviando MSG_GAME_STARTED a todos los jugadores..." << std::endl;
    if (!broadcast_callback) return;
    std::vector<uint8_t> buffer = { static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED) };
    broadcast_callback(buffer, -1);
}

void Match::stop_match() {
    is_active.store(false);
    if (gameloop) gameloop->stop_match();
}
Match::~Match() {
    stop_match();
    if (gameloop) gameloop->join();
}
void Match::print_players_info() const {
    std::cout << "Match status info printed.\n";
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