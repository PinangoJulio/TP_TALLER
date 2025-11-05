#ifndef DTOS_H
#define DTOS_H

#include <cstdint>
#include <string>

// ============================================
// DTOs de la Fase de Juego
// ============================================

// Comandos del Cliente → Servidor (Juego)
enum class GameCommand : uint8_t {
    ACCELERATE = 0x01,
    BRAKE = 0x02,
    TURN_LEFT = 0x03,
    TURN_RIGHT = 0x04,
    USE_NITRO = 0x05,
    DISCONNECT = 0xFF
};

// ✅ AGREGAR: Tipos de mensajes del servidor
enum class ServerMessageType : uint8_t {
    MSG_SERVER = 0x01,
    NITRO_ON = 0x02,
    NITRO_OFF = 0x03
};

// Estructura de comando con ID del jugador
struct Command {
    GameCommand action;
    uint16_t player_id; 
};

// Estado de un auto (para enviar al cliente)
struct CarState {
    uint16_t player_id;
    float pos_x;
    float pos_y;
    float angle;
    float velocity;
    uint8_t health;
    bool nitro_active;
} __attribute__((packed));

// Estructura de mensaje del servidor
struct ServerMsg {
    uint8_t type;
    uint16_t cars_with_nitro;
    uint8_t nitro_status;
} __attribute__((packed));

// ============================================
// Constantes útiles
// ============================================
#define QUIT 'q'
#define MAX_PLAYERS 8

#endif // DTOS_H