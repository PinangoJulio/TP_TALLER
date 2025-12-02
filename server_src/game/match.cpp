#include "match.h"

#include <arpa/inet.h>  // htons, htonl
#include <cstring>      // memset, strncpy
#include <iomanip>
#include <utility>

#include "common_src/config.h"
#include "common_src/dtos.h"  // RaceInfoDTO, ServerMessageType
#define RUTA_MAPS "server_src/city_maps/"

// ============================================
Match::Match(std::string host_name, int code, int max_players)
    : host_name(std::move(host_name)), match_code(code), is_active(false),
      state(MatchState::WAITING), players_queues(), command_queue(),
      max_players(max_players) {

    // ✅ CREAR E INICIAR GAMELOOP INMEDIATAMENTE
    std::cout << "[Match] Creado: " << this->host_name << " (code=" << code
              << ", max_players=" << max_players << ")\n";

    std::cout << "[Match] >>> Creando GameLoop...\n";
    gameloop = std::make_unique<GameLoop>(command_queue, players_queues);

    std::cout << "[Match] >>> Iniciando GameLoop (thread)...\n";
    gameloop->start();

    std::cout << "[Match] ✅ GameLoop iniciado y esperando jugadores\n";
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


// SELECCIÓN DE AUTO


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
            break;
        }
    }

    std::cout << "[Match] Jugador " << it->second.name << " eligió auto " << car_name << " ("
              << car_type << ")\n";

    // REGISTRAR JUGADOR EN GAMELOOP CON SU AUTO
    if (gameloop) {
        std::cout << "[Match] >>> Registrando " << it->second.name << " en GameLoop con auto "
                  << car_name << "\n";
        gameloop->add_player(player_id, it->second.name, car_name, car_type);
        std::cout << "[Match] ✅ Jugador " << it->second.name << " agregado al GameLoop exitosamente\n";
    } else {
        std::cerr << "[Match] ❌ ERROR: GameLoop no existe, no se pudo registrar jugador\n";
    }

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

    if (ready && it->second.car_name.empty()) {
        std::cout << "[Match] " << it->second.name << " tried to ready without selecting car\n";
        return false;
    }

    it->second.is_ready = ready;
    std::cout << "[Match] " << it->second.name << " is now " << (ready ? "READY" : "NOT READY")
              << "\n";


    gameloop->set_player_ready(player_id, ready);


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
// CARRERAS
// ============================================
std::vector<std::string> Match::get_race_yaml_paths() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mtx));
    std::vector<std::string> paths;
    paths.reserve(race_configs.size());
    for (const auto& cfg : race_configs) {
        paths.push_back(std::string(RUTA_MAPS) + cfg.city + "/" + cfg.race_name + ".yaml");
    }
    return paths;
}

void Match::set_race_configs(const std::vector<ServerRaceConfig>& configs) {
    std::lock_guard<std::mutex> lock(mtx);
    race_configs = configs;

    // ✅ Crear las races y agregarlas al GameLoop
    std::vector<std::unique_ptr<Race>> new_races;

    for (size_t i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        std::string yaml_path = "server_src/city_maps/" + config.city + "/" + config.race_name + ".yaml";

        int race_number = static_cast<int>(new_races.size()) + 1;
        auto new_race = std::make_unique<Race>(config.city, yaml_path, race_number);
        new_races.push_back(std::move(new_race));

        std::cout << "[Match] Carrera agregada: " << config.city << " -> " << yaml_path << "\n";
    }

    // Transferir las carreras al GameLoop (ya existe desde el constructor)
    gameloop->set_races(std::move(new_races));

    for (const auto& [id, info] : players_info) {
        if (!info.car_name.empty() && !info.car_type.empty()) {
            std::cout << "[Match] >>> Agregando jugador " << info.name << " al GameLoop\n";
            gameloop->add_player(id, info.name, info.car_name, info.car_type);
        }
    }

    std::cout << "[Match] Configuradas " << race_configs.size() << " carreras en GameLoop\n";
}

// void Match::send_game_started_confirmation() {
//     std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
//     std::cout << "[Match]  Enviando MSG_GAME_STARTED a todos los jugadores..." << std::endl;

//     if (!broadcast_callback) {
//         std::cerr << "[Match] ❌ No hay broadcast_callback configurado" << std::endl;
//         return;
//     }

//     // Serializar mensaje simple: solo el tipo
//     std::vector<uint8_t> buffer;
//     buffer.push_back(static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED)); // 0x14

//     std::cout << "[Match]   Mensaje serializado: 0x" << std::hex 
//               << static_cast<int>(buffer[0]) << std::dec << " (size: " << buffer.size() << ")" << std::endl;

//     // Enviar a TODOS los jugadores (sin excluir a nadie)
//     std::cout << "[Match]   Llamando a broadcast_callback para " << players_info.size() 
//               << " jugadores..." << std::endl;
    
//     broadcast_callback(buffer, -1);

//     std::cout << "[Match] ✅ MSG_GAME_STARTED broadcast completado" << std::endl;
//     std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
// }

void Match::send_game_started_confirmation() {
    std::cout << "[Match] ⚠️ send_game_started_confirmation() deprecated - use receiver.cpp instead" << std::endl;
    // Ya no enviamos nada aquí, se maneja en receiver.cpp
}

void Match::send_race_info_to_all_players() {
    if (race_configs.empty()) {
        std::cerr << "[Match] ❌ No hay carreras configuradas para enviar info" << std::endl;
        return;
    }

    const auto& first_race_config = race_configs[0];

    RaceInfoDTO race_info;
    std::memset(&race_info, 0, sizeof(race_info));

    // Copiar ciudad
    std::strncpy(race_info.city_name, first_race_config.city.c_str(),
                 sizeof(race_info.city_name) - 1);

    // Copiar nombre de carrera
    std::strncpy(race_info.race_name, first_race_config.race_name.c_str(),
                 sizeof(race_info.race_name) - 1);

    // Construir ruta del mapa
    std::string map_path = "server_src/city_maps/" + first_race_config.city + "/" +
                          first_race_config.race_name;
    std::strncpy(race_info.map_file_path, map_path.c_str(), sizeof(race_info.map_file_path) - 1);

    // Configuración numérica
    race_info.total_laps = 3;  // TODO: Obtener de config
    race_info.race_number = 1;
    race_info.total_races = static_cast<uint8_t>(race_configs.size());
    race_info.total_checkpoints = 0;  // TODO: Obtener del mapa parseado
    race_info.max_time_ms = 600000;   // 10 minutos en ms

    std::cout << "[Match] Enviando info de carrera a todos los jugadores..." << std::endl;
    std::cout << "[Match]   Ciudad: " << race_info.city_name << std::endl;
    std::cout << "[Match]   Carrera: " << race_info.race_name << std::endl;
    std::cout << "[Match]   Mapa: " << race_info.map_file_path << std::endl;

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

// ============================================
// INICIO DE LA PARTIDA
// ============================================
/*
 * FLUJO DE INICIO:
 *
 * 1. Lobby → Todos los jugadores marcan "listo"
 * 2. Host presiona "INICIAR" → llega MSG_START_GAME
 * 3. Receiver llama MatchesMonitor::start_match()
 * 4. MatchesMonitor llama Match::start_match() ← AQUÍ ESTAMOS
 *
 * ESTE MÉTODO:
 * ✅ Valida que todos estén listos
 * ✅ Envía info de la PRIMERA carrera a los clientes (mapa YAML, ciudad, etc.)
 * ✅ Notifica al GameLoop que comience la primera carrera
 *
 * GAMELOOP:
 * - YA ESTÁ CORRIENDO desde el constructor de Match
 * - Ya tiene los jugadores registrados (se agregaron en select_car)
 * - Ya tiene las carreras configuradas (se agregaron en configure_races)
 * - Estaba esperando en run() con current_race_index paused
 * - Ahora comenzará la CARRERA 0 (primera)
 * - Posiciona autos en spawn_points
 * - Espera comandos (ACCELERATE, BRAKE, etc.)
 * - Envía snapshots a clientes cada 250ms
 */

void Match::start_match() {
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
    std::cout << "[Match] 🏁 Petición de START recibida para Match " << match_code << std::endl;

    // 1. Evitar doble inicio
    if (state == MatchState::STARTED) {
        std::cout << "[Match] ⚠️ La partida ya está iniciada." << std::endl;
        return;
    }

    // 2. Validación Crítica: ¿Están todos listos y hay mapa?
    if (!can_start()) {
        std::cout << "[Match] ❌ No se puede iniciar: Faltan jugadores o no están todos listos." << std::endl;
        // Opcional: Podrías enviar un mensaje de error al cliente que intentó iniciar
        return;
    }

    // 3. Cambiar estado
    state = MatchState::STARTED;
    is_active.store(true);

    std::cout << "[Match] ✅ Condiciones cumplidas. Iniciando simulación..." << std::endl;

    // 4. Despertar al GameLoop
    // Esto hace que GameLoop salga de su pausa, ejecute reset_players_for_race()
    // y asigne las posiciones (x, y) de spawn correctamente.
    if (gameloop) {
        gameloop->start_game();
    } else {
        std::cerr << "[Match] ❌ ERROR CRÍTICO: GameLoop es null" << std::endl;
        return;
    }
    
    // 5. Avisar a TODOS los clientes (Broadcast)
    // Esto envía el MSG_GAME_STARTED (0x14) que hace que los clientes cierren Qt y abran SDL.
    // Al abrir SDL, recibirán el primer snapshot con las posiciones ya calculadas.
  //  send_game_started_confirmation(); 

    std::cout << "[Match] 🚀 Clientes notificados. ¡Juego en marcha!" << std::endl;
    std::cout << "[Match] ════════════════════════════════════════════" << std::endl;
}

void Match::stop_match() {
    std::cout << "[Match] Deteniendo partida..." << std::endl;
    is_active.store(false);

    if (gameloop && gameloop->is_alive()) {
        gameloop->stop_match();
        gameloop->join();
    }
}

Match::~Match() {
    is_active = false;
    gameloop->join();
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
