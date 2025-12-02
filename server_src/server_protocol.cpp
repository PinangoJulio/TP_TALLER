#include "server_protocol.h"

#include <netinet/in.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

ServerProtocol::ServerProtocol(Socket& skt) : socket(skt) {}

// --- Lectura básica de datos ---

uint8_t ServerProtocol::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0)
        throw std::runtime_error("Connection closed");
    return type;
}

std::string ServerProtocol::read_string() {
    uint16_t len_net;
    int bytes_read = socket.recvall(&len_net, sizeof(len_net));
    if (bytes_read == 0)
        throw std::runtime_error("Connection closed while reading string length");

    uint16_t len = ntohs(len_net);
    if (len == 0 || len > 1024)
        throw std::runtime_error("Invalid string length: " + std::to_string(len));

    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    return std::string(buffer.begin(), buffer.end());
}

uint16_t ServerProtocol::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void ServerProtocol::send_buffer(const std::vector<uint8_t>& buffer) {
    socket.sendall(buffer.data(), buffer.size());
}

uint8_t ServerProtocol::get_uint8_t() {
    uint8_t n;
    socket.recvall(&n, sizeof(n));
    return n;
}

/** lee comandos que el cliente envia durante la fase de juego. Lee los datos del socket y los
 interpreta segun el codigo de comando recibido **/
bool ServerProtocol::read_command_client(ComandMatchDTO& command) {
    // Leer el código de comando (1 byte)
    uint8_t cmd_code;
    int bytes = socket.recvall(&cmd_code, sizeof(cmd_code));
    if (bytes == 0) return false; // conexión cerrada
    if (bytes < 0) return false;  // error de lectura
    // Log desactivado para reducir spam en producción
    // std::cout << "[ServerProtocol] Reading command code: 0x" << std::hex << (int)cmd_code << std::dec << std::endl;

    // Interpretar según el código y leer datos adicionales si es necesario
    switch (cmd_code) {
    // ===== COMANDOS SIMPLES (solo 1 byte) =====
        case CMD_ACCELERATE:
            // std::cout << "[ServerProtocol] Reading command code: ACCELERATE" << std::endl;
        command.command = GameCommand::ACCELERATE;
        command.speed_boost = 1.0f;
        break;

        case CMD_BRAKE:  //(frenar)
            // std::cout << "[ServerProtocol] Reading command code: BRAKE" << std::endl;
        command.command = GameCommand::BRAKE;
        command.speed_boost = 1.0f;
        break;

    // ===== MOVIMIENTO EN 4 DIRECCIONES FIJAS =====
    case CMD_MOVE_UP:
        command.command = GameCommand::MOVE_UP;
        command.speed_boost = 1.0f;
        break;

    case CMD_MOVE_DOWN:
        command.command = GameCommand::MOVE_DOWN;
        command.speed_boost = 1.0f;
        break;

    case CMD_MOVE_LEFT:
        command.command = GameCommand::MOVE_LEFT;
        command.speed_boost = 1.0f;
        break;

    case CMD_MOVE_RIGHT:
        command.command = GameCommand::MOVE_RIGHT;
        command.speed_boost = 1.0f;
        break;

    case CMD_USE_NITRO:
        command.command = GameCommand::USE_NITRO;
        break;

    case CMD_STOP_ALL:
        command.command = GameCommand::STOP_ALL;
        break;

    case CMD_DISCONNECT:
        command.command = GameCommand::DISCONNECT;
        break;

    // ===== CHEATS SIMPLES =====
    case CMD_CHEAT_INVINCIBLE:
        command.command = GameCommand::CHEAT_INVINCIBLE;
        break;

    case CMD_CHEAT_WIN_RACE:
        command.command = GameCommand::CHEAT_WIN_RACE;
        break;

    case CMD_CHEAT_LOSE_RACE:
        command.command = GameCommand::CHEAT_LOSE_RACE;
        break;

    case CMD_CHEAT_MAX_SPEED:
        command.command = GameCommand::CHEAT_MAX_SPEED;
        break;

    case CMD_TURN_LEFT: {
        // std::cout << "[ServerProtocol] Reading command code: TURN_LEFT" << std::endl;
        command.command = GameCommand::TURN_LEFT;
        // Leer intensidad del giro (1 byte: 0-100 = 0.0-1.0)
        uint8_t intensity;
        socket.recvall(&intensity, sizeof(intensity));
        command.turn_intensity = static_cast<float>(intensity) / 100.0f;
        break;
    }

    case CMD_TURN_RIGHT: {
        // std::cout << "[ServerProtocol] Reading command code: TURN_RIGHT" << std::endl;
        command.command = GameCommand::TURN_RIGHT;
        // Leer intensidad del giro (1 byte: 0-100 = 0.0-1.0)
        uint8_t intensity;
        socket.recvall(&intensity, sizeof(intensity));
        command.turn_intensity = static_cast<float>(intensity) / 100.0f;
        break;
    }

    case CMD_CHEAT_TELEPORT: {
        command.command = GameCommand::CHEAT_TELEPORT_CHECKPOINT;
        // Leer ID del checkpoint (2 bytes)
        uint16_t checkpoint_id;
        socket.recvall(&checkpoint_id, sizeof(checkpoint_id));
        command.checkpoint_id = ntohs(checkpoint_id);
        break;
    }

    // ===== UPGRADES (código + nivel + costo) =====
    case CMD_UPGRADE_SPEED: {
        command.command = GameCommand::UPGRADE_SPEED;
        command.upgrade_type = UpgradeType::SPEED;
        // Leer nivel (1 byte)
        socket.recvall(&command.upgrade_level, sizeof(command.upgrade_level));
        // Leer costo en ms (2 bytes)
        uint16_t cost_net;
        socket.recvall(&cost_net, sizeof(cost_net));
        command.upgrade_cost_ms = ntohs(cost_net);
        break;
    }

    case CMD_UPGRADE_ACCEL: {
        command.command = GameCommand::UPGRADE_ACCELERATION;
        command.upgrade_type = UpgradeType::ACCELERATION;
        socket.recvall(&command.upgrade_level, sizeof(command.upgrade_level));
        uint16_t cost_net;
        socket.recvall(&cost_net, sizeof(cost_net));
        command.upgrade_cost_ms = ntohs(cost_net);
        break;
    }

    case CMD_UPGRADE_HANDLING: {
        command.command = GameCommand::UPGRADE_HANDLING;
        command.upgrade_type = UpgradeType::HANDLING;
        socket.recvall(&command.upgrade_level, sizeof(command.upgrade_level));
        uint16_t cost_net;
        socket.recvall(&cost_net, sizeof(cost_net));
        command.upgrade_cost_ms = ntohs(cost_net);
        break;
    }

    case CMD_UPGRADE_DURABILITY: {
        command.command = GameCommand::UPGRADE_DURABILITY;
        command.upgrade_type = UpgradeType::DURABILITY;
        socket.recvall(&command.upgrade_level, sizeof(command.upgrade_level));
        uint16_t cost_net;
        socket.recvall(&cost_net, sizeof(cost_net));
        command.upgrade_cost_ms = ntohs(cost_net);
        break;
    }

    default:
        std::cerr << "[ServerProtocol] Código de comando desconocido: 0x" << std::hex
                  << static_cast<int>(cmd_code) << std::dec << std::endl;
        break;
    }

    return true;
}

// ============================================================================
// FUNCIONES HELPER PARA SERIALIZACIÓN
// ============================================================================

static void push_back_uint16_t(std::vector<uint8_t>& buffer, uint16_t value) {
    uint16_t net_value = htons(value);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[1]);
}

static void push_back_uint32_t(std::vector<uint8_t>& buffer, uint32_t value) {
    uint32_t net_value = htonl(value);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[1]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[2]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[3]);
}

static void push_back_int32_t(std::vector<uint8_t>& buffer, int32_t value) {
    uint32_t net_value = htonl(static_cast<uint32_t>(value));
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[0]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[1]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[2]);
    buffer.push_back(reinterpret_cast<uint8_t*>(&net_value)[3]);
}

static void push_back_string(std::vector<uint8_t>& buffer, const std::string& str) {
    // Enviar longitud (uint16_t)
    push_back_uint16_t(buffer, static_cast<uint16_t>(str.size()));
    // Enviar contenido
    buffer.insert(buffer.end(), str.begin(), str.end());
}

bool ServerProtocol::send_client_id(int client_id) {
    uint16_t id = htons(static_cast<uint16_t>(client_id));
    socket.sendall(&id, sizeof(id));
    return true;
}



// ============================================================================
// SERIALIZACIÓN DEL GAMESTATE (SNAPSHOT)
// ============================================================================



bool ServerProtocol::send_snapshot(const GameState& snapshot) {
    std::vector<uint8_t> buffer;
    buffer.reserve(4096);

    buffer.push_back(static_cast<uint8_t>(ServerMessageType::GAME_STATE_UPDATE));

    // ---- 1. PLAYERS ----
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.players.size()));

    for (const InfoPlayer& p : snapshot.players) {
        // Strings
        push_back_string(buffer, p.username);
        push_back_string(buffer, p.car_name);
        push_back_string(buffer, p.car_type);

        // player_id
        push_back_uint16_t(buffer, static_cast<uint16_t>(p.player_id));

        // pos_x / pos_y (int32 scaled x100)
        push_back_int32_t(buffer, static_cast<int32_t>(p.pos_x * 100.0f));
        push_back_int32_t(buffer, static_cast<int32_t>(p.pos_y * 100.0f));

        // angle / speed (uint16 scaled)
        push_back_uint16_t(buffer, static_cast<uint16_t>(p.angle * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(p.speed * 100.0f));

        // velocity_x / velocity_y (int16 scaled)
        push_back_int32_t(buffer, static_cast<int32_t>(p.velocity_x * 100.0f));
        push_back_int32_t(buffer, static_cast<int32_t>(p.velocity_y * 100.0f));


        // health, nitro (uint8)
        buffer.push_back((uint8_t)p.health);
        buffer.push_back((uint8_t)p.nitro_amount);

        // flags
        uint8_t flags = 0;
        flags |= p.nitro_active ? 0x01 : 0;
        flags |= p.is_drifting  ? 0x02 : 0;
        flags |= p.is_colliding ? 0x04 : 0;
        buffer.push_back(flags);

        // progreso
        push_back_uint16_t(buffer, p.completed_laps);
        push_back_uint16_t(buffer, p.current_checkpoint);
        buffer.push_back((uint8_t)p.position_in_race);

        // race time
        push_back_uint32_t(buffer, p.race_time_ms);

        // estados
        buffer.push_back(p.race_finished ? 1 : 0);
        buffer.push_back(p.is_alive ? 1 : 0);
        buffer.push_back(p.disconnected ? 1 : 0);
    }

    // ---- 2. CHECKPOINTS ----
    push_back_uint16_t(buffer, snapshot.checkpoints.size());
    for (const CheckpointInfo& c : snapshot.checkpoints) {
        push_back_uint32_t(buffer, c.id);
        push_back_int32_t(buffer, (int32_t)(c.pos_x * 100.0f));
        push_back_int32_t(buffer, (int32_t)(c.pos_y * 100.0f));
        push_back_uint16_t(buffer, (uint16_t)(c.width * 100.0f));
        push_back_uint16_t(buffer, (uint16_t)(c.angle * 100.0f));

        buffer.push_back(c.is_start ? 1 : 0);
        buffer.push_back(c.is_finish ? 1 : 0);
    }

    // ---- 3. NPCs ----
    push_back_uint16_t(buffer, snapshot.npcs.size());
    for (const NPCCarInfo& n : snapshot.npcs) {
        push_back_uint32_t(buffer, n.npc_id);
        push_back_int32_t(buffer, (int32_t)(n.pos_x * 100.0f));
        push_back_int32_t(buffer, (int32_t)(n.pos_y * 100.0f));
        push_back_uint16_t(buffer, (uint16_t)(n.angle * 100.0f));
        push_back_uint16_t(buffer, (uint16_t)(n.speed * 100.0f));

        buffer.push_back(n.is_parked ? 1 : 0);
    }

    // ---- 4. RACE INFO ----
    buffer.push_back((uint8_t)snapshot.race_info.status);
    buffer.push_back((uint8_t)snapshot.race_info.race_number);
    buffer.push_back((uint8_t)snapshot.race_info.total_races);

    push_back_uint32_t(buffer, snapshot.race_info.remaining_time_ms);

    buffer.push_back((uint8_t)snapshot.race_info.players_finished);
    buffer.push_back((uint8_t)snapshot.race_info.total_players);

    // ---- 5. EVENTS ----
    push_back_uint16_t(buffer, snapshot.events.size());

    for (const GameEvent& e : snapshot.events) {
        buffer.push_back((uint8_t)e.type);
        push_back_uint32_t(buffer, e.player_id);
        push_back_int32_t(buffer, (int32_t)(e.pos_x * 100.0f));
        push_back_int32_t(buffer, (int32_t)(e.pos_y * 100.0f));
    }

    // ---- SEND ----
    return socket.sendall(buffer.data(), buffer.size());
}




// ============================================================================
// ENVÍO DE INFORMACIÓN DE CARRERA
// ============================================================================

bool ServerProtocol::send_race_info(const RaceInfoDTO& race_info) {
    std::vector<uint8_t> buffer;

    // 1. Tipo de mensaje
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::RACE_INFO));

    // 2. Ciudad (string con longitud)
    std::string city_str(race_info.city_name);
    push_back_string(buffer, city_str);

    // 3. Nombre de carrera (string con longitud)
    std::string race_str(race_info.race_name);
    push_back_string(buffer, race_str);

    // 4. Ruta del mapa (string con longitud)
    std::string map_str(race_info.map_file_path);
    push_back_string(buffer, map_str);

    // 5. Datos numéricos
    buffer.push_back(race_info.total_laps);
    buffer.push_back(race_info.race_number);
    buffer.push_back(race_info.total_races);

    push_back_uint16_t(buffer, race_info.total_checkpoints);
    push_back_uint32_t(buffer, race_info.max_time_ms);

    // ENVIAR BUFFER
    socket.sendall(buffer.data(), buffer.size());

    return true;
}

// ENVIAR RUTAS YAML DE LAS CARRERAS

bool ServerProtocol::send_race_paths(const std::vector<std::string>& yaml_paths) {
    std::vector<uint8_t> buffer;

    // 1. Tipo de mensaje
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::RACE_PATHS));

    // 2. Cantidad de carreras (uint8_t porque no habrá más de 255 carreras por partida)
    buffer.push_back(static_cast<uint8_t>(yaml_paths.size()));

    // 3. Cada path como string
    for (const auto& path : yaml_paths) {
        push_back_string(buffer, path);
    }

    // 4. Enviar buffer
    socket.sendall(buffer.data(), buffer.size());

    std::cout << "[ServerProtocol] ✅ Sent " << yaml_paths.size()
              << " race paths to client" << std::endl;

    return true;
}
