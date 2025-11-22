#include "match.h"

#include <iomanip>
#include "common_src/config.h"

Match::Match(std::string host_name, int code, int max_players)
        : host_name(std::move(host_name)),
          match_code(code),
          is_active(false),
          state(MatchState::WAITING),
          current_race_index(0),
          players_queues(),
          command_queue(),
          max_players(max_players){}

// ============================================
// GESTIÓN DE JUGADORES
// ============================================

bool Match::can_player_join_match() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return static_cast<int>(players_info.size()) < max_players && state != MatchState::STARTED;
}

bool Match::add_player(int id, std::string nombre, Queue<GameState> &queue_enviadora) {
    std::lock_guard<std::mutex> lock(mtx);

    // Validar sin llamar a can_player_join_match() para evitar deadlock
    if (static_cast<int>(players_info.size()) >= max_players || state == MatchState::STARTED) {
        return false;
    }

    // Agregar a la queue de broadcast
    players_queues.add_client_queue(queue_enviadora, id);

    // Crear info de lobby
    PlayerLobbyInfo info;
    info.id = id;
    info.name = nombre;
    info.is_ready = false;
    info.sender_queue = &queue_enviadora;

    players_info[id] = info;
    player_name_to_id[nombre] = id;

    // Mantener compatibilidad con vector de players
    auto player = std::make_unique<Player>(id, nombre);
    players.push_back(std::move(player));

    // Actualizar estado
    if (static_cast<int>(players_info.size()) >= 2) {
        state = MatchState::READY;
    }

    std::cout << "[Match] Jugador agregado: " << nombre << " (id=" << id
              << ", total=" << players_info.size() << ")\n";

    // debug imprimir lista de jgugadores
    print_players_info();

    return true;
}

bool Match::remove_player(int id_jugador) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = players_info.find(id_jugador);
    if (it == players_info.end()) return false;

    std::string nombre = it->second.name;

    // Eliminar de todas las estructuras
    players_info.erase(it);
    player_name_to_id.erase(nombre);
    players_queues.delete_client_queue(id_jugador);

    // Eliminar del vector de players
    auto player_it = std::remove_if(players.begin(), players.end(),
                                     [id_jugador](const std::unique_ptr<Player>& p) {
                                         return p->getId() == id_jugador;
                                     });
    players.erase(player_it, players.end());

    // Actualizar estado
    if (static_cast<int>(players_info.size()) < 2) {
        state = MatchState::WAITING;
    }

    std::cout << "[Match] Jugador eliminado: " << nombre << " (id=" << id_jugador
              << ", restantes=" << players_info.size() << ")" << std::endl;

    // ✅ Notificar a los demás jugadores si hay callback de broadcast
    if (broadcast_callback) {
        // Crear mensaje de notificación (se implementará en el receiver)
        // broadcast_callback se encargará de enviar el mensaje
    }

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

    // Actualizar en Player también
    for (auto& player : players) {
        if (player->getId() == player_id) {
            player->setSelectedCar(car_name);
            player->getCarType(car_type);
            break;
        }
    }

    std::cout << "[Match] Jugador " << it->second.name
              << " eligió auto " << car_name << " (" << car_type << ")\n";

    // ✅ IMPRIMIR LISTA DE JUGADORES
    print_players_info();

    return true;
}

bool Match::set_player_car_by_name(const std::string& player_name, const std::string& car_name, const std::string& car_type) {
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

    // Validar que tenga auto seleccionado
    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] " << it->second.name << " tried to ready without selecting car\n";
        return false;
    }

    it->second.is_ready = ready;
    std::cout << "[Match] " << it->second.name << " is now "
              << (ready ? "READY" : "NOT READY") << "\n";

    // ✅ IMPRIMIR LISTA DE JUGADORES
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

// ============================================
// SNAPSHOT
// ============================================

std::map<int, PlayerLobbyInfo> Match::get_players_snapshot() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    return players_info;
}

const std::map<int, PlayerLobbyInfo>& Match::get_players() const {
    // No lock porque es un const reference, el caller debe hacer lock si necesita
    return players_info;
}

// ============================================
// CARRERAS
// ============================================

void Match::set_race_configs(const std::vector<RaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs;

    // Crear las races
    for (const auto& config : configs) {
        std::string yaml_path = "server_src/city_maps/" + config.city + "/" + config.map;
        add_race(yaml_path, config.city);
    }

    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras\n";
}

void Match::add_race(const std::string& yaml_path, const std::string& city_name) {
    races.push_back(std::make_unique<Race>(command_queue, players_queues, city_name, yaml_path));
    std::cout << "[Match] Carrera agregada: " << city_name << " -> " << yaml_path << "\n";
}

void Match::start_match() {
    std::lock_guard<std::mutex> lock(mtx);

    if (state == MatchState::STARTED) {
        std::cout << "[Match] Ya está iniciada\n";
        return;
    }

    if (!can_start()) {
        std::cout << "[Match] No se puede iniciar (no todos listos o sin carreras)\n";
        return;
    }

    state = MatchState::STARTED;
    std::cout << "[Match] ✅ Partida iniciada con " << players_info.size() << " jugadores\n";

    // Los jugadores se registran en start_next_race() para cada carrera
    // Iniciar primera carrera
    start_next_race();
}

void Match::start_next_race() {
    if (races.empty()) {
        std::cerr << "[Match] No hay carreras configuradas.\n";
        return;
    }
    if (current_race_index >= static_cast<int>(races.size())) {
        std::cout << "[Match] Todas las carreras finalizadas.\n";
        is_active.store(false);
        return;
    }

    is_active.store(true);
    auto& current_race = races[current_race_index];
    std::string yaml = current_race->get_map_path();
    std::cout << "[Match] Iniciando carrera #" << current_race_index + 1
              << " en mapa " << yaml << "\n";

    // ✅ REGISTRAR TODOS LOS JUGADORES EN EL GAMELOOP
    std::cout << "[Match] Registrando " << players_info.size() << " jugadores en GameLoop...\n";
    for (const auto& [id, info] : players_info) {
        current_race->add_player_to_gameloop(id, info.name, info.car_name, info.car_type);
    }

    // ✅ Configurar parámetros de la carrera
    current_race->set_total_laps(3);  // TODO: Obtener de configuración
    current_race->set_city(current_race->get_city_name());

    // Iniciar la carrera
    current_race->start();
    current_race_index++;
}

Match::~Match() {
    is_active = false;
    races.clear();
}

// ============================================
// DEBUG: IMPRIMIR INFORMACIÓN DE JUGADORES
// ============================================

void Match::print_players_info() const {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MATCH #" << match_code << " - JUGADORES (" << players_info.size() << "/" << max_players << ")";
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

    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
}
