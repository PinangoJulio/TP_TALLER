#ifndef DTOS_H
#define DTOS_H

#include <cstdint>
#include <string>

// ============================================
// DTOs del Lobby (ya existentes en lobby_protocol.h)
// ============================================
// (No duplicar GameInfo, ya está en lobby_protocol.h)

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

// Snapshot completo del juego (broadcast a todos los clientes)
struct GameSnapshot {
    uint32_t frame_number;
    uint16_t num_cars;
    // Seguido de num_cars × CarState
} __attribute__((packed));

// Evento especial (choques, explosiones, checkpoints)
enum class GameEventType : uint8_t {
    CAR_COLLISION = 0x01,
    CHECKPOINT_CROSSED = 0x02,
    CAR_EXPLODED = 0x03,
    RACE_FINISHED = 0x04
};

struct GameEvent {
    GameEventType type;
    uint16_t player_id;
    uint16_t data;  // Contexto adicional (ej: ID del checkpoint)
} __attribute__((packed));

// ============================================
// Constantes útiles
// ============================================
#define QUIT 'q'
#define MAX_PLAYERS 8

#endif // DTOS_H