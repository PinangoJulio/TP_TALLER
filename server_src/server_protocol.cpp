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
    if (bytes == 0) {
        return false;  // Connection closed
    }

    // Interpretar según el código y leer datos adicionales si es necesario
    switch (cmd_code) {
    // ===== COMANDOS SIMPLES (solo 1 byte) =====
    case CMD_ACCELERATE:
        command.command = GameCommand::ACCELERATE;
        command.speed_boost = 1.0f;
        break;

    case CMD_BRAKE:  //(frenar)
        command.command = GameCommand::BRAKE;
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
        command.command = GameCommand::TURN_LEFT;
        // Leer intensidad del giro (1 byte: 0-100 = 0.0-1.0)
        uint8_t intensity;
        socket.recvall(&intensity, sizeof(intensity));
        command.turn_intensity = static_cast<float>(intensity) / 100.0f;
        break;
    }

    case CMD_TURN_RIGHT: {
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
        command.command = GameCommand::DISCONNECT;
        return false;
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

static void push_back_string(std::vector<uint8_t>& buffer, const std::string& str) {
    // Enviar longitud (uint16_t)
    push_back_uint16_t(buffer, static_cast<uint16_t>(str.size()));
    // Enviar contenido
    buffer.insert(buffer.end(), str.begin(), str.end());
}

// ============================================================================
// SERIALIZACIÓN DEL GAMESTATE (SNAPSHOT)
// ============================================================================

bool ServerProtocol::send_snapshot(const GameState& snapshot) {
    // Verificar que haya jugadores
    if (snapshot.players.empty()) {
        std::cerr << "[ServerProtocol] Warning: Empty snapshot, nothing to send." << std::endl;
        return false;
    }

    std::vector<uint8_t> buffer;

    // 1. JUGADORES
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.players.size()));

    for (const InfoPlayer& player : snapshot.players) {
        // ID del jugador (4 bytes)
        push_back_uint32_t(buffer, static_cast<uint32_t>(player.player_id));

        // Nombre (string con longitud)
        push_back_string(buffer, player.username);

        // Auto (nombre y tipo)
        push_back_string(buffer, player.car_name);
        push_back_string(buffer, player.car_type);

        // Posición y física (multiplicar por 100 para enviar como enteros)
        push_back_uint32_t(buffer, static_cast<uint32_t>(player.pos_x * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(player.pos_y * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(player.angle * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(player.speed * 100.0f));

        // Velocidad (componentes X e Y, pueden ser negativos, usar int16_t)
        int16_t vel_x = static_cast<int16_t>(player.velocity_x * 100.0f);
        int16_t vel_y = static_cast<int16_t>(player.velocity_y * 100.0f);
        push_back_uint16_t(buffer, static_cast<uint16_t>(vel_x));
        push_back_uint16_t(buffer, static_cast<uint16_t>(vel_y));

        // Estado del auto
        buffer.push_back(static_cast<uint8_t>(player.health));
        buffer.push_back(static_cast<uint8_t>(player.nitro_amount));
        buffer.push_back(player.nitro_active ? 0x01 : 0x00);
        buffer.push_back(player.is_drifting ? 0x01 : 0x00);
        buffer.push_back(player.is_colliding ? 0x01 : 0x00);

        // Progreso en la carrera
        push_back_uint16_t(buffer, static_cast<uint16_t>(player.completed_laps));
        push_back_uint16_t(buffer, static_cast<uint16_t>(player.current_checkpoint));
        buffer.push_back(static_cast<uint8_t>(player.position_in_race));
        push_back_uint32_t(buffer, static_cast<uint32_t>(player.race_time_ms));

        // Estado del jugador
        buffer.push_back(player.race_finished ? 0x01 : 0x00);
        buffer.push_back(player.is_alive ? 0x01 : 0x00);
        buffer.push_back(player.disconnected ? 0x01 : 0x00);
    }

    // 2. CHECKPOINTS
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.checkpoints.size()));

    for (const CheckpointInfo& cp : snapshot.checkpoints) {
        push_back_uint32_t(buffer, static_cast<uint32_t>(cp.id));
        push_back_uint32_t(buffer, static_cast<uint32_t>(cp.pos_x * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(cp.pos_y * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(cp.width * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(cp.angle * 100.0f));
        buffer.push_back(cp.is_start ? 0x01 : 0x00);
        buffer.push_back(cp.is_finish ? 0x01 : 0x00);
    }

    // 3. HINTS (FLECHAS DIRECCIONALES)
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.hints.size()));

    for (const HintInfo& hint : snapshot.hints) {
        push_back_uint32_t(buffer, static_cast<uint32_t>(hint.id));
        push_back_uint32_t(buffer, static_cast<uint32_t>(hint.pos_x * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(hint.pos_y * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(hint.direction_angle * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(hint.for_checkpoint));
    }

    // 4. NPCs
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.npcs.size()));

    for (const NPCCarInfo& npc : snapshot.npcs) {
        push_back_uint32_t(buffer, static_cast<uint32_t>(npc.npc_id));
        push_back_uint32_t(buffer, static_cast<uint32_t>(npc.pos_x * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(npc.pos_y * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(npc.angle * 100.0f));
        push_back_uint16_t(buffer, static_cast<uint16_t>(npc.speed * 100.0f));
        push_back_string(buffer, npc.car_model);
        buffer.push_back(npc.is_parked ? 0x01 : 0x00);
    }

    // 5. RACE CURRENT INFO
    push_back_string(buffer, snapshot.race_current_info.city);
    push_back_string(buffer, snapshot.race_current_info.race_name);
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.race_current_info.total_laps));
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.race_current_info.total_checkpoints));

    // 6. RACE INFO
    buffer.push_back(static_cast<uint8_t>(snapshot.race_info.status));
    buffer.push_back(static_cast<uint8_t>(snapshot.race_info.race_number));
    buffer.push_back(static_cast<uint8_t>(snapshot.race_info.total_races));
    push_back_uint32_t(buffer, static_cast<uint32_t>(snapshot.race_info.remaining_time_ms));
    buffer.push_back(static_cast<uint8_t>(snapshot.race_info.players_finished));
    buffer.push_back(static_cast<uint8_t>(snapshot.race_info.total_players));
    push_back_string(buffer, snapshot.race_info.winner_name);

    // 7. EVENTOS
    push_back_uint16_t(buffer, static_cast<uint16_t>(snapshot.events.size()));

    for (const GameEvent& event : snapshot.events) {
        buffer.push_back(static_cast<uint8_t>(event.type));
        push_back_uint32_t(buffer, static_cast<uint32_t>(event.player_id));
        push_back_uint32_t(buffer, static_cast<uint32_t>(event.pos_x * 100.0f));
        push_back_uint32_t(buffer, static_cast<uint32_t>(event.pos_y * 100.0f));
    }

    // ENVIAR BUFFER
    socket.sendall(buffer.data(), buffer.size());

    return true;
}
