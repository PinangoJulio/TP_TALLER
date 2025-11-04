#ifndef GAME_PROTOCOL_H
#define GAME_PROTOCOL_H

#include <vector>
#include <cstdint>
#include "dtos.h"

// Tipos de mensajes del juego
enum GameMessageType : uint8_t {
    // Cliente → Servidor
    MSG_GAME_COMMAND = 0x20,
    
    // Servidor → Cliente
    MSG_GAME_SNAPSHOT = 0x30,
    MSG_GAME_EVENT = 0x31,
    MSG_RACE_START = 0x32,
    MSG_RACE_END = 0x33
};

namespace GameProtocol {
    // Serialización Cliente → Servidor
    std::vector<uint8_t> serialize_command(const Command& cmd);
    
    // Serialización Servidor → Cliente
    std::vector<uint8_t> serialize_snapshot(const GameSnapshot& snapshot, 
                                            const std::vector<CarState>& cars);
    std::vector<uint8_t> serialize_event(const GameEvent& event);
    std::vector<uint8_t> serialize_race_start();
    std::vector<uint8_t> serialize_race_end(const std::vector<uint16_t>& rankings);
}

#endif // GAME_PROTOCOL_H
