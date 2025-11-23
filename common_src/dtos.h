#ifndef DTOS_H
#define DTOS_H

#include <cstdint>
#include <string>

// ============================================
// Constantes útiles

#define QUIT        'q'
#define MAX_PLAYERS 8

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
    ERR_NOT_HOST = 0x06,
    ERR_NOT_ENOUGH_PLAYERS = 0x07,
    ERR_PLAYER_NOT_IN_GAME = 0x08,
    ERR_INVALID_CAR_INDEX = 0x09,
    ERR_PLAYERS_NOT_READY = 0x0A
};

struct PlayerRoomState {
    char username[32];
    char car_name[32];
    char car_type[16];
    uint8_t is_ready;  // 1 = listo, 0 = no listo
} __attribute__((packed));

// DTOs de la Fase de Juego
// ============================================

// Códigos de comando para el protocolo binario (1 byte)
#define CMD_ACCELERATE 0x01
#define CMD_BRAKE      0x02
#define CMD_TURN_LEFT  0x03
#define CMD_TURN_RIGHT 0x04
#define CMD_USE_NITRO  0x05
#define CMD_STOP_ALL   0x30
#define CMD_DISCONNECT 0xFF

// Códigos de cheats
#define CMD_CHEAT_INVINCIBLE 0x10
#define CMD_CHEAT_WIN_RACE   0x11
#define CMD_CHEAT_LOSE_RACE  0x12
#define CMD_CHEAT_MAX_SPEED  0x13
#define CMD_CHEAT_TELEPORT   0x14

// Códigos de upgrades (entre carreras)
#define CMD_UPGRADE_SPEED      0x20
#define CMD_UPGRADE_ACCEL      0x21
#define CMD_UPGRADE_HANDLING   0x22
#define CMD_UPGRADE_DURABILITY 0x23

// Comandos que el cliente envía al servidor durante la carrera
enum class GameCommand : uint8_t {
    // Movimiento básico
    ACCELERATE = CMD_ACCELERATE,
    BRAKE = CMD_BRAKE,
    TURN_LEFT = CMD_TURN_LEFT,
    TURN_RIGHT = CMD_TURN_RIGHT,

    // Power-ups y habilidades
    USE_NITRO = CMD_USE_NITRO,

    // Cheats (para desarrollo/testing)
    CHEAT_INVINCIBLE = CMD_CHEAT_INVINCIBLE,
    CHEAT_WIN_RACE = CMD_CHEAT_WIN_RACE,
    CHEAT_LOSE_RACE = CMD_CHEAT_LOSE_RACE,
    CHEAT_MAX_SPEED = CMD_CHEAT_MAX_SPEED,
    CHEAT_TELEPORT_CHECKPOINT = CMD_CHEAT_TELEPORT,

    // Mejoras de auto (entre carreras)
    UPGRADE_SPEED = CMD_UPGRADE_SPEED,
    UPGRADE_ACCELERATION = CMD_UPGRADE_ACCEL,
    UPGRADE_HANDLING = CMD_UPGRADE_HANDLING,
    UPGRADE_DURABILITY = CMD_UPGRADE_DURABILITY,

    // Control
    STOP_ALL = CMD_STOP_ALL,
    DISCONNECT = CMD_DISCONNECT
};

// Tipos de mensajes que el servidor envía a los clientes
enum class ServerMessageType : uint8_t {
    GAME_STATE_UPDATE = 0x01,   // Estado completo del juego
    RACE_STARTED = 0x02,        // La carrera comenzó
    RACE_FINISHED = 0x03,       // La carrera terminó
    CHECKPOINT_CROSSED = 0x04,  // Jugador cruzó un checkpoint
    COLLISION_EVENT = 0x05,     // Hubo una colisión
    CAR_DESTROYED = 0x06,       // Un auto fue destruido
    NITRO_ACTIVATED = 0x07,     // Nitro activado
    NITRO_DEACTIVATED = 0x08,   // Nitro desactivado
    POSITION_UPDATE = 0x09,     // Actualización de posiciones en carrera
    LAP_COMPLETED = 0x0A,       // Vuelta completada
    COUNTDOWN = 0x0B,           // Countdown antes de iniciar
    RACE_TIMEOUT = 0x0C         // Carrera terminó por timeout
};

// Tipo de colisión
enum class CollisionType : uint8_t {
    CAR_VS_WALL = 0x01,
    CAR_VS_CAR = 0x02,
    CAR_VS_NPC = 0x03,
    CAR_VS_OBSTACLE = 0x04
};

// Severidad del choque (para animaciones y efectos)
enum class CollisionSeverity : uint8_t {
    MINOR = 0x01,   // Roce lateral
    MEDIUM = 0x02,  // Impacto moderado
    MAJOR = 0x03    // Impacto frontal/explosion
};

// Tipo de upgrade
enum class UpgradeType : uint8_t {
    SPEED = 0x01,
    ACCELERATION = 0x02,
    HANDLING = 0x03,
    DURABILITY = 0x04
};

// DTO principal para comandos del juego
// Los campos se llenan según el comando recibido
struct ComandMatchDTO {
    uint16_t player_id;   // ID del jugador
    GameCommand command;  // Comando a ejecutar

    // Campos opcionales (se usan según el comando)
    float turn_intensity;      // Para TURN_LEFT/TURN_RIGHT (0.0 - 1.0)
    float speed_boost;         // Para ACCELERATE/BRAKE (0.0 - 1.0)
    uint16_t checkpoint_id;    // Para CHEAT_TELEPORT_CHECKPOINT
    UpgradeType upgrade_type;  // Para UPGRADEs
    uint8_t upgrade_level;     // Para UPGRADEs (nivel 1, 2, 3...)
    uint16_t upgrade_cost_ms;  // Para UPGRADEs (penalización en ms)

    // Constructor por defecto
    ComandMatchDTO()
        : player_id(0), command(GameCommand::DISCONNECT), turn_intensity(0.0f), speed_boost(0.0f),
          checkpoint_id(0), upgrade_type(UpgradeType::SPEED), upgrade_level(0), upgrade_cost_ms(0) {}
};

// Estado de un auto en la carrera (para enviar al cliente)
struct CarState {
    uint16_t player_id;
    float pos_x;
    float pos_y;
    float angle;           // En radianes
    float velocity;        // Velocidad actual
    float velocity_x;      // Componente X de velocidad
    float velocity_y;      // Componente Y de velocidad
    uint8_t health;        // 0-100
    uint8_t nitro_amount;  // 0-100
    bool nitro_active;
    bool is_drifting;
    bool is_colliding;
    bool is_destroyed;
    uint16_t current_checkpoint;  // Índice del siguiente checkpoint a cruzar
    uint8_t current_lap;          // Vuelta actual
    uint8_t position_in_race;     // 1st, 2nd, 3rd, etc.
} __attribute__((packed));

// Evento de colisión (servidor → cliente)
struct CollisionEvent {
    uint16_t player_id_1;  // Primer auto involucrado
    uint16_t player_id_2;  // Segundo auto (0 si es pared/obstáculo)
    CollisionType type;
    CollisionSeverity severity;
    float impact_speed;  // Velocidad del impacto
    float damage_dealt;  // Daño causado
    float pos_x;         // Posición del impacto
    float pos_y;
} __attribute__((packed));

// Resultado de una carrera
struct RaceResult {
    uint16_t player_id;
    uint32_t finish_time_ms;  // Tiempo en milisegundos
    int32_t penalties_ms;     // Penalizaciones en ms
    uint32_t final_time_ms;   // Tiempo final = finish_time - penalties
    uint8_t position;         // Posición final (1st, 2nd, etc.)
    bool finished;            // Si terminó o fue descalificado
    char player_name[32];
} __attribute__((packed));

// Snapshot completo del estado del juego (servidor → cliente)
struct GameStateSnapshot {
    uint32_t timestamp_ms;        // Timestamp del servidor
    uint8_t num_cars;             // Cantidad de autos en la carrera
    uint8_t race_status;          // 0=waiting, 1=countdown, 2=racing, 3=finished
    uint16_t countdown_seconds;   // Segundos restantes de countdown
    uint32_t elapsed_time_ms;     // Tiempo transcurrido de carrera
    uint16_t current_checkpoint;  // Checkpoint actual del jugador
    uint16_t total_checkpoints;   // Total de checkpoints en el circuito
    // Seguido por CarState[num_cars]
} __attribute__((packed));

#endif  // DTOS_H
