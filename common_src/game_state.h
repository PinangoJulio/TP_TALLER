#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations (las clases del servidor se incluirán en el .cpp)
class Player;

// CONFIGURACIÓN DE PARTIDA (usado en el lobby)

// Estados generales de la partida
enum class MatchStatus : uint8_t {
    WAITING_FOR_PLAYERS = 0,
    IN_PROGRESS = 1,
    FINISHED = 2
};

// Configuración de una carrera
struct ServerRaceConfig {
    std::string city;       // "Vice City", "Liberty City", "San Andreas"
    std::string race_name;  // "Playa", "Centro", "Desierto"
};

// Información de jugador en el lobby
struct PlayerInfoLobby {
    int32_t player_id = 0;
    std::string username;
    std::string car_name;
    std::string car_type;
};

// Configuración completa de la partida (Match)
struct MatchConfig {
    std::string match_name;               // Nombre de la partida
    uint8_t max_players = 8;              // Máximo 8 jugadores
    std::vector<ServerRaceConfig> races;  // Lista de carreras
    std::vector<PlayerInfoLobby> players_cfg;
};

// SNAPSHOT DEL JUEGO (enviado continuamente del servidor a los clientes)

// ---- Información detallada de cada jugador ----
struct InfoPlayer {
    // Identificación
    int32_t player_id = 0;
    std::string username;
    std::string car_name;
    std::string car_type;

    // Posición y física
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float angle = 0.0f;
    float speed = 0.0f;
    float velocity_x = 0.0f;
    float velocity_y = 0.0f;

    // Estado del vehículo
    float health = 100.0f;
    float nitro_amount = 100.0f;
    bool nitro_active = false;

    // ✅ NUEVO: Nivel del jugador (0 = calle, 1 = puente)
    int level = 0;

    // Progreso en la carrera
    int completed_laps = 0;
    int current_checkpoint = 0;
    int position_in_race = 0;
    uint32_t race_time_ms = 0;

    // Estados
    bool is_drifting = false;
    bool is_colliding = false;
    bool is_alive = true;
    bool race_finished = false;
    bool disconnected = false;
};

// ---- Información de checkpoints ----
struct CheckpointInfo {
    int id = 0;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float width = 50.0f;     // Ancho del checkpoint
    float angle = 0.0f;      // Ángulo de rotación
    bool is_start = false;   // Punto de partida
    bool is_finish = false;  // Meta
};

// ---- Hints/flechas direccionales ----
struct HintInfo {
    int id = 0;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float direction_angle = 0.0f;  // Hacia dónde apunta la flecha
    int for_checkpoint = 0;    // A qué checkpoint conduce
};

// ---- NPCs (autos que no son jugadores) ----
struct NPCCarInfo {
    int npc_id = 0;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float angle = 0.0f;
    float speed = 0.0f;
    std::string car_model = "sedan";  // Tipo visual
    bool is_parked = false;           // Estacionado o circulando
};

// ---- Información del circuito/mapa actual ----
struct RaceCurrentInfo {
    std::string city;               // Ciudad actual
    std::string race_name;          // Nombre del circuito
    int total_laps = 3;         // Vueltas totales
    int total_checkpoints = 0;  // Cantidad de checkpoints
};

// ---- Estado global de la carrera ----
struct RaceInfo {
    MatchStatus status = MatchStatus::WAITING_FOR_PLAYERS;
    int race_number = 1;             // Carrera actual (1, 2, 3...)
    int total_races = 3;             // Carreras totales en la partida
    int32_t remaining_time_ms = 600000;  // Tiempo restante (10 min max)
    int players_finished = 0;
    int total_players = 0;
    std::string winner_name;
};

// ---- Eventos especiales (explosiones, colisiones) ----
struct GameEvent {
    enum EventType : uint8_t {
        EXPLOSION = 1,
        HEAVY_COLLISION = 2,
        PLAYER_FINISHED = 3,
        CHECKPOINT_REACHED = 4
    };

    EventType type = EXPLOSION;
    int player_id = 0;  // Jugador involucrado
    float pos_x = 0.0f;
    float pos_y = 0.0f;
};

// SNAPSHOT COMPLETO

struct GameState {
    // Jugadores
    std::vector<InfoPlayer> players;

    // Mapa
    std::vector<CheckpointInfo> checkpoints;
    std::vector<HintInfo> hints;
    std::vector<NPCCarInfo> npcs;

    // Información de la carrera
    RaceCurrentInfo race_current_info;
    RaceInfo race_info;

    // Eventos recientes (explosiones, etc.)
    std::vector<GameEvent> events;

    // ---- Constructores ----
    GameState() = default;

    // Constructor que llena el snapshot desde el servidor
    // La implementación está en common_src/game_state.cpp
    GameState(const std::vector<Player*>& players, const std::string& city,
              const std::string& map_path, bool running);

    // ---- Buscar jugador por ID ----
    InfoPlayer* findPlayer(int id) const {
        for (auto& p : players) {
            if (p.player_id == id)
                return const_cast<InfoPlayer*>(&p);
        }
        return nullptr;
    }
};

#endif  // GAME_STATE_H_