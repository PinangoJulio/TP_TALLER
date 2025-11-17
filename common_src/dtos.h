#ifndef DTOS_H
#define DTOS_H

#include <cstdint>
#include <string>

// ============================================
// Constantes útiles
// ============================================
#define QUIT 'q'
#define MAX_PLAYERS 8

// ============================================
// DTOs de la Fase del Lobby
// ============================================
enum LobbyMessageType : uint8_t {
    // Cliente → Servidor
    MSG_USERNAME = 0x01,
    MSG_LIST_GAMES = 0x02,
    MSG_CREATE_GAME = 0x03,
    MSG_JOIN_GAME = 0x04,
    MSG_START_GAME = 0x05,
    MSG_SELECT_CAR = 0x06,
    MSG_LEAVE_GAME = 0x07,
    MSG_PLAYER_READY = 0x08,

    // Servidor → Cliente
    MSG_WELCOME = 0x10,
    MSG_GAMES_LIST = 0x11,
    MSG_GAME_CREATED = 0x12,
    MSG_GAME_JOINED = 0x13,
    MSG_GAME_STARTED = 0x14,
    MSG_CITY_MAPS = 0x15,
    MSG_CAR_SELECTED_ACK = 0x16,
    
    // NOTIFICACIONES PUSH (Servidor → Todos en la sala)
    MSG_PLAYER_JOINED_NOTIFICATION = 0x20,   
    MSG_PLAYER_LEFT_NOTIFICATION = 0x21,     
    MSG_PLAYER_READY_NOTIFICATION = 0x22,   
    MSG_CAR_SELECTED_NOTIFICATION = 0x23,    
    MSG_ROOM_STATE_UPDATE = 0x24,
    MSG_ROOM_SNAPSHOT = 0x25,           

    MSG_ERROR = 0xFF
};

// Códigos de error
enum LobbyErrorCode : uint8_t {
    ERR_GAME_NOT_FOUND = 0x01,
    ERR_GAME_FULL = 0x02,
    ERR_INVALID_USERNAME = 0x03,
    ERR_GAME_ALREADY_STARTED = 0x04,
    ERR_ALREADY_IN_GAME = 0x05,
    ERR_NOT_HOST = 0x06,  // 🔥 NOTA: Mantener el código por compatibilidad, pero no se usa
    ERR_NOT_ENOUGH_PLAYERS = 0x07,
    ERR_PLAYER_NOT_IN_GAME = 0x08,
    ERR_INVALID_CAR_INDEX = 0x09,
    ERR_PLAYERS_NOT_READY = 0x0A  
};

// 🔥 CORREGIDO: Eliminar is_host del struct
struct PlayerRoomState {
    char username[32];
    char car_name[32];
    char car_type[16];
    uint8_t is_ready;   // 1 = listo, 0 = no listo
    // 🔥 ELIMINADO: uint8_t is_host;
} __attribute__((packed));

// ============================================
// DTOs de la Fase de Juego
// ============================================

enum class GameCommand : uint8_t {
    ACCELERATE = 0x01,
    BRAKE = 0x02,
    TURN_LEFT = 0x03,
    TURN_RIGHT = 0x04,
    USE_NITRO = 0x05,
    DISCONNECT = 0xFF
};

enum class ServerMessageType : uint8_t {
    MSG_SERVER = 0x01,
    NITRO_ON = 0x02,
    NITRO_OFF = 0x03
};

struct Command {
    GameCommand action;
    uint16_t player_id; 
};

struct CarState {
    uint16_t player_id;
    float pos_x;
    float pos_y;
    float angle;
    float velocity;
    uint8_t health;
    bool nitro_active;
} __attribute__((packed));

struct ComandMatchDTO {
    int player_id;
    GameCommand command;

    ComandMatchDTO() : player_id(0), command(GameCommand::DISCONNECT) {}
};

#endif // DTOS_H