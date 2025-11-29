#include "server_protocol.h"

#include <netinet/in.h>

#include <cstring>
#include <iostream>
#include <stdexcept>

#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

// ✅ NO incluir game_protocol.h, solo definir el valor que necesitamos
#define MSG_GAME_SNAPSHOT 0x30  // Tipo de mensaje para snapshots del juego

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

bool ServerProtocol::read_command_client(ComandMatchDTO& command) {
    // ... (tu código existente sin cambios)
    uint8_t cmd_code;
    int bytes = socket.recvall(&cmd_code, sizeof(cmd_code));
    if (bytes == 0) {
        return false;
    }

    switch (cmd_code) {
    case CMD_ACCELERATE:
        command.command = GameCommand::ACCELERATE;
        command.speed_boost = 1.0f;
        break;

    case CMD_BRAKE:
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
        uint8_t intensity;
        socket.recvall(&intensity, sizeof(intensity));
        command.turn_intensity = static_cast<float>(intensity) / 100.0f;
        break;
    }

    case CMD_TURN_RIGHT: {
        command.command = GameCommand::TURN_RIGHT;
        uint8_t intensity;
        socket.recvall(&intensity, sizeof(intensity));
        command.turn_intensity = static_cast<float>(intensity) / 100.0f;
        break;
    }

    case CMD_CHEAT_TELEPORT: {
        command.command = GameCommand::CHEAT_TELEPORT_CHECKPOINT;
        uint16_t checkpoint_id;
        socket.recvall(&checkpoint_id, sizeof(checkpoint_id));
        command.checkpoint_id = ntohs(checkpoint_id);
        break;
    }

    case CMD_UPGRADE_SPEED: {
        command.command = GameCommand::UPGRADE_SPEED;
        command.upgrade_type = UpgradeType::SPEED;
        socket.recvall(&command.upgrade_level, sizeof(command.upgrade_level));
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
    push_back_uint16_t(buffer, static_cast<uint16_t>(str.size()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

bool ServerProtocol::send_client_id(int client_id) {
    uint16_t id = htons(static_cast<uint16_t>(client_id));
    socket.sendall(&id, sizeof(id));
    return true;
}

// ============================================================================
// SERIALIZACIÓN SIMPLIFICADA DEL GAMESTATE (SNAPSHOT)
// Coincide EXACTAMENTE con client_protocol.cpp::receive_snapshot()
// ============================================================================

bool ServerProtocol::send_snapshot(const GameState& snapshot) {
    if (snapshot.players.empty()) {
        std::cerr << "[ServerProtocol] Warning: Empty snapshot, nothing to send." << std::endl;
        return false;
    }

    std::vector<uint8_t> buffer;

    // ✅ 1. Tipo de mensaje (1 byte) - 0x30 = MSG_GAME_SNAPSHOT
    buffer.push_back(MSG_GAME_SNAPSHOT);

    // 2. Número de jugadores (1 byte)
    buffer.push_back(static_cast<uint8_t>(snapshot.players.size()));

    // 3. Datos de cada jugador (FORMATO SIMPLIFICADO)
    for (const InfoPlayer& player : snapshot.players) {
        // player_id (2 bytes - uint16_t)
        uint16_t player_id_net = htons(static_cast<uint16_t>(player.player_id));
        buffer.push_back(reinterpret_cast<uint8_t*>(&player_id_net)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&player_id_net)[1]);

        // pos_x (4 bytes - uint32_t, multiplicado por 100)
        uint32_t pos_x_net = htonl(static_cast<uint32_t>(player.pos_x * 100.0f));
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_x_net)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_x_net)[1]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_x_net)[2]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_x_net)[3]);

        // pos_y (4 bytes - uint32_t, multiplicado por 100)
        uint32_t pos_y_net = htonl(static_cast<uint32_t>(player.pos_y * 100.0f));
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_y_net)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_y_net)[1]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_y_net)[2]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&pos_y_net)[3]);

        // angle (2 bytes - uint16_t, multiplicado por 100)
        uint16_t angle_net = htons(static_cast<uint16_t>(player.angle * 100.0f));
        buffer.push_back(reinterpret_cast<uint8_t*>(&angle_net)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&angle_net)[1]);

        // speed (2 bytes - uint16_t, multiplicado por 10)
        uint16_t speed_net = htons(static_cast<uint16_t>(player.speed * 10.0f));
        buffer.push_back(reinterpret_cast<uint8_t*>(&speed_net)[0]);
        buffer.push_back(reinterpret_cast<uint8_t*>(&speed_net)[1]);

        // health (1 byte)
        buffer.push_back(static_cast<uint8_t>(player.health));

        // ✅ level (1 byte) - CRÍTICO PARA RENDERIZAR PUENTES
        buffer.push_back(static_cast<uint8_t>(player.level));

        // flags (1 byte combinado)
        uint8_t flags = 0;
        if (player.nitro_active)   flags |= 0x01;
        if (player.is_drifting)    flags |= 0x02;
        if (player.race_finished)  flags |= 0x04;
        if (player.is_alive)       flags |= 0x08;
        buffer.push_back(flags);
    }

    // ENVIAR TODO EL BUFFER
    socket.sendall(buffer.data(), buffer.size());

    std::cout << "[ServerProtocol] 📤 Snapshot enviado: " 
              << snapshot.players.size() << " jugadores, "
              << buffer.size() << " bytes" << std::endl;

    return true;
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