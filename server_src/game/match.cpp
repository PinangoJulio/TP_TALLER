#include "match.h"

#include <arpa/inet.h>  // htons, htonl
#include <cstring>      // memset, strncpy
#include <iomanip>
#include <utility>
#include <algorithm>    // remove_if

#include "common_src/config.h"
#include "common_src/dtos.h"  // RaceInfoDTO, ServerMessageType

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
    if (it == players_info.end())
        return false;

    std::string nombre = it->second.name;

    // Eliminar de todas las estructuras
    players_info.erase(it);
    player_name_to_id.erase(nombre);
    players_queues.delete_client_queue(id_jugador);

    // Eliminar del vector de players
    auto player_it = std::remove_if(
        players.begin(), players.end(),
        [id_jugador](const std::unique_ptr<Player>& p) { return p->getId() == id_jugador; });
    players.erase(player_it, players.end());

    // Actualizar estado
    if (static_cast<int>(players_info.size()) < 2) {
        state = MatchState::WAITING;
    }

    std::cout << "[Match] Jugador eliminado: " << nombre << " (id=" << id_jugador
              << ", restantes=" << players_info.size() << ")" << std::endl;

    // Notificar a los demás jugadores si hay callback de broadcast
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
    if (it == players_info.end())
        return false;

    it->second.car_name = car_name;
    it->second.car_type = car_type;

    // Actualizar en Player también
    for (auto& player : players) {
        if (player->getId() == player_id) {
            player->setSelectedCar(car_name);
            // El car_type se establecerá cuando se cree el Car en GameLoop::add_player()
            break;
        }
    }

    std::cout << "[Match] Jugador " << it->second.name << " eligió auto " << car_name << " ("
              << car_type << ")\n";

    // REGISTRAR EN GAMELOOP INMEDIATAMENTE (para todas las carreras configuradas)
    if (!races.empty()) {
        std::cout << "[Match] >>> Registrando " << it->second.name << " en los GameLoops de las "
                  << races.size() << " carreras..." << std::endl;

        for (auto& race : races) {
            race->add_player_to_gameloop(player_id, it->second.name, car_name, car_type);
        }

        std::cout << "[Match] <<< " << it->second.name << " registrado en todas las carreras"
                  << std::endl;
    } else {
        std::cout
            << "[Match]   No hay carreras configuradas aún, jugador se agregará cuando se configuren"
            << std::endl;
    }

    //  IMPRIMIR LISTA DE JUGADORES
    print_players_info();

    return true;
}

bool Match::set_player_car_by_name(const std::string& player_name, const std::string& car_name,
                                   const std::string& car_type) {
    int player_id = get_player_id_by_name(player_name);
    if (player_id == -1)
        return false;
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
        if (info.car_name.empty())
            return false;
    }
    return !players_info.empty();
}

// ============================================
// ESTADO READY
// ============================================

bool Match::set_player_ready(int player_id, bool ready) {
    std::lock_guard<std::mutex> lock(mtx);

    auto it = players_info.find(player_id);
    if (it == players_info.end())
        return false;

    // Validar que tenga auto seleccionado
    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] " << it->second.name << " tried to ready without selecting car\n";
        return false;
    }

    it->second.is_ready = ready;
    std::cout << "[Match] " << it->second.name << " is now " << (ready ? "READY" : "NOT READY")
              << "\n";

    // ✅ PROPAGAR A TODOS LOS GAMELOOPS
    if (!races.empty()) {
        std::cout << "[Match] >>> Actualizando estado ready en GameLoops..." << std::endl;
        for (auto& race : races) {
            race->set_player_ready(player_id, ready);
        }
        std::cout << "[Match] <<< Estado ready actualizado en " << races.size() << " carreras"
                  << std::endl;
    }

    // ✅ IMPRIMIR LISTA DE JUGADORES
    print_players_info();

    return true;
}

bool Match::set_player_ready_by_name(const std::string& player_name, bool ready) {
    int player_id = get_player_id_by_name(player_name);
    if (player_id == -1)
        return false;
    return set_player_ready(player_id, ready);
}

bool Match::is_player_ready(int player_id) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    auto it = players_info.find(player_id);
    return (it != players_info.end()) ? it->second.is_ready : false;
}

bool Match::all_players_ready() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    if (players_info.empty())
        return false;

    for (const auto& [id, info] : players_info) {
        if (!info.is_ready)
            return false;
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
// CARRERAS (LÓGICA ACTUALIZADA)
// ============================================

void Match::set_race_configs(const std::vector<ServerRaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs; // Guardar la configuración de secuencia

    // Crear las races
    races.clear();
    for (const auto& config : configs) {
        // Se asume la ruta: "server_src/city_maps/Vice City/Centro.yaml"
        std::string yaml_path = "server_src/city_maps/" + config.city + "/" + config.race_name + ".yaml";
        add_race(yaml_path, config.city);
    }

    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras\n";
}

void Match::add_race(const std::string& yaml_path, const std::string& city_name) {
    auto new_race = std::make_unique<Race>(command_queue, players_queues, city_name, yaml_path);

    // ✅ Agregar jugadores que ya seleccionaron auto a esta nueva carrera
    for (const auto& [id, info] : players_info) {
        if (!info.car_name.empty() && !info.car_type.empty()) {
            std::cout << "[Match] >>> Agregando " << info.name << " retroactivamente a carrera "
                      << city_name << std::endl;
            new_race->add_player_to_gameloop(id, info.name, info.car_name, info.car_type);
        }
    }

    races.push_back(std::move(new_race));
    std::cout << "[Match] Carrera agregada: " << city_name << " -> " << yaml_path << "\n";
}

// ============================================
// ENVÍO DE INFORMACIÓN DE CARRERA
// ============================================

void Match::send_race_info_to_all_players(int race_idx) {
    if (race_idx < 0 || race_idx >= static_cast<int>(race_configs.size())) {
        std::cerr << "[Match] ❌ Índice de carrera inválido para enviar info: " << race_idx << std::endl;
        return;
    }

    // Obtener información de la carrera específica usando el índice
    const auto& config = race_configs[race_idx];
    const auto& race = races[race_idx];

    // Crear el DTO con la información de la carrera
    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));

    // Copiar ciudad
    std::strncpy(race_info.city_name, config.city.c_str(),
                 sizeof(race_info.city_name) - 1);

    // Copiar nombre de carrera
    std::strncpy(race_info.race_name, config.race_name.c_str(),
                 sizeof(race_info.race_name) - 1);

    // Copiar ruta del mapa
    std::string map_path = race->get_map_path();
    std::strncpy(race_info.map_file_path, map_path.c_str(), sizeof(race_info.map_file_path) - 1);

    // Configuración numérica
    race_info.total_laps = config.laps;  // <-- USAR LAPS DE LA CONFIGURACIÓN (1 para Sprint)
    race_info.race_number = race_idx + 1;
    race_info.total_races = static_cast<uint8_t>(races.size());
    race_info.total_checkpoints = 0;  // TODO: Obtener del mapa parseado
    race_info.max_time_ms = 600000;   // 10 minutos en ms

    std::cout << "[Match] 📨 Enviando info de carrera #" << (race_idx + 1) << " a todos..." << std::endl;
    std::cout << "[Match]   Ciudad: " << race_info.city_name << std::endl;
    std::cout << "[Match]   Carrera: " << race_info.race_name << std::endl;
    std::cout << "[Match]   Laps: " << (int)race_info.total_laps << std::endl;

    if (!broadcast_callback) {
        std::cerr << "[Match] ❌ No hay broadcast_callback configurado" << std::endl;
        return;
    }

    // Serializar RaceInfoDTO manualmente
    std::vector<uint8_t> buffer;

    // 1. Tipo de mensaje
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::RACE_INFO));

    // 2. Ciudad (string con longitud)
    std::string city_str(race_info.city_name);
    uint16_t city_len = htons(static_cast<uint16_t>(city_str.size()));
    buffer.push_back(reinterpret_cast<uint8_t*>(&city_len)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&city_len)[1]);
    buffer.insert(buffer.end(), city_str.begin(), city_str.end());

    // 3. Nombre de carrera (string con longitud)
    std::string race_str(race_info.race_name);
    uint16_t race_len = htons(static_cast<uint16_t>(race_str.size()));
    buffer.push_back(reinterpret_cast<uint8_t*>(&race_len)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&race_len)[1]);
    buffer.insert(buffer.end(), race_str.begin(), race_str.end());

    // 4. Ruta del mapa (string con longitud)
    std::string map_str(race_info.map_file_path);
    uint16_t map_len = htons(static_cast<uint16_t>(map_str.size()));
    buffer.push_back(reinterpret_cast<uint8_t*>(&map_len)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&map_len)[1]);
    buffer.insert(buffer.end(), map_str.begin(), map_str.end());

    // 5. Datos numéricos
    buffer.push_back(race_info.total_laps);
    buffer.push_back(race_info.race_number);
    buffer.push_back(race_info.total_races);

    uint16_t checkpoints = htons(race_info.total_checkpoints);
    buffer.push_back(reinterpret_cast<uint8_t*>(&checkpoints)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&checkpoints)[1]);

    uint32_t max_time = htonl(race_info.max_time_ms);
    buffer.push_back(reinterpret_cast<uint8_t*>(&max_time)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&max_time)[1]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&max_time)[2]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&max_time)[3]);

    // Enviar a todos los jugadores (sin excluir a nadie)
    broadcast_callback(buffer, -1);

    std::cout << "[Match] ✅ Información de carrera enviada a " << players_info.size()
              << " jugadores" << std::endl;
}

// ============================================
// INICIO DE PARTIDA
// ============================================

void Match::start_match() {
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
    std::cout << "[Match] 🏁 INICIANDO CAMPEONATO CODE " << match_code << std::endl;
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;

    std::lock_guard<std::mutex> lock(mtx);

    if (state == MatchState::STARTED) {
        std::cout << "[Match]   Ya está iniciada" << std::endl;
        return;
    }

    if (!can_start()) {
        std::cout << "[Match]  No se puede iniciar (no todos listos o sin carreras)" << std::endl;
        return;
    }

    state = MatchState::STARTED;
    current_race_index = 0; // Reiniciar índice

    std::cout << "[Match]  Partida iniciada con " << players_info.size() << " jugadores"
              << std::endl;
    std::cout << "[Match]  Carreras configuradas: " << races.size() << std::endl;

    // Arrancar la secuencia llamando a start_next_race()
    std::cout << "[Match] >>> Llamando a start_next_race()..." << std::endl;
    start_next_race();
    std::cout << "[Match] <<< start_next_race() completado" << std::endl;
}

void Match::start_next_race() {
    if (races.empty()) {
        std::cerr << "[Match] No hay carreras configuradas.\n";
        return;
    }
    
    // Verificar si ya terminamos todas las carreras
    if (current_race_index >= static_cast<int>(races.size())) {
        std::cout << "[Match] 🏆 CAMPEONATO FINALIZADO (Todas las carreras completadas).\n";
        is_active.store(false);
        state = MatchState::FINISHED;
        return;
    }

    is_active.store(true);
    
    // Obtener la instancia de carrera y su configuración
    auto& current_race = races[current_race_index];
    const auto& current_config = race_configs[current_race_index];
    
    std::string yaml = current_race->get_map_path();
    
    std::cout << "[Match] Iniciando carrera #" << (current_race_index + 1) << " en " 
              << current_config.city << " - " << current_config.race_name 
              << " (" << current_config.laps << " laps)\n";

    // ✅ Enviar info de ESTA carrera a los clientes
    send_race_info_to_all_players(current_race_index);

    // ✅ Configurar parámetros de la carrera en el GameLoop
    current_race->set_total_laps(current_config.laps); 
    current_race->set_city(current_config.city);

    // Iniciar la carrera
    current_race->start();
    
    // Preparar índice para la próxima (cuando esta termine)
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

    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
}