#include "server_protocol.h"
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <netinet/in.h>
#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

ServerProtocol::ServerProtocol(Socket &skt)
    : socket(skt) {}

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


/** lee comandos que el cliente envia durante la fase de juego. Lee los datos del socket y los interpreta
 segun el codigo de comando recibido **/
bool ServerProtocol::read_command_client(ComandMatchDTO& command) {
    // Leer el código de comando (1 byte)
    uint8_t cmd_code;
    int bytes = socket.recvall(&cmd_code, sizeof(cmd_code));
    if (bytes == 0) {
        return false; // Connection closed
    }

    // Interpretar según el código y leer datos adicionales si es necesario
    switch(cmd_code) {
        // ===== COMANDOS SIMPLES (solo 1 byte) =====
        case CMD_ACCELERATE:
            command.command = GameCommand::ACCELERATE;
            command.speed_boost = 1.0f;
            break;

        case CMD_BRAKE: //(frenar)
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
            std::cerr << "[ServerProtocol] Código de comando desconocido: 0x"
                      << std::hex << static_cast<int>(cmd_code) << std::dec << std::endl;
            command.command = GameCommand::DISCONNECT;
            return false;
    }

    return true;
}
