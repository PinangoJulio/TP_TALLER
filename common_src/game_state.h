#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "server_src/game/player.h"

// No incluimos el Player real para evitar dependencias circulares.
// Si prefieres, puedes incluir "../server_src/game/player.h" en la unidad .cpp
// forward declaration: se usa en constructor que recibe Player*

// Estados generales de la partida (lobby/match)
enum class MatchStatus : int {
    WAITING_FOR_PLAYERS = 0,
    IN_PROGRESS = 1,
    FINISHED = 2
};

// Config de una sola carrera (elección host: ciudad + sector/mapa)
struct RaceConfig {
    std::string city;  // Ej: "Vice City"
    std::string map;   // Ej: "Centro" o "Puentes"
};

// Config general de la partida (lo que se define en el lobby)
struct PlayerInfo {
    int32_t player_id = 0;  // id del jugador dentro de la partida (generado por server)
    std::string username;   // nombre del jugador
    std::string car_name;   // modelo elegido, p.ej. "Coupe"
    std::string car_type;   // p.ej. "Deportivo", "Camioneta" (opcional)
};

struct MatchConfig {
    std::string name;               // nombre de la partida
    std::uint8_t max_players = 8;   // cupo máximo
    std::vector<RaceConfig> races;  // lista de carreras elegidas por el host (pueden repetirse)
    std::vector<PlayerInfo>
        players_cfg;  // lista de jugadores unidos con su elección de auto (lobby)
};

// ======= Estructuras para el snapshot (GameState) que se transmite a los clientes =======

// Información por cada jugador en el snapshot de carrera (estado dinámico)
struct InfoPlayer {
    int32_t player_id = 0;
    std::string username;
    std::string car_name;
    std::string car_type;

    // Posición/estado de la carrera
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    float angle = 0.0f;
    float speed = 0.0f;

    int32_t completed_laps = 0;

    // Flags de estado
    bool is_drifting = false;
    bool is_colliding = false;
    bool race_finished = false;
    bool disconnected = false;
};

// Checkpoint / meta
struct CheckpointInfo {
    int32_t id = 0;
    float pos_x = 0.0f;
    float pos_y = 0.0f;
    bool reached = false;
};

// Datos del track
struct TrackInfo {
    std::string name;
    int32_t total_laps = 0;
    float total_length = 0.0f;
};

// Estado global de la carrera
struct RaceInfo {
    MatchStatus status = MatchStatus::WAITING_FOR_PLAYERS;
    int32_t remaining_time_seconds = 0;
    int32_t players_finished = 0;
    int32_t total_players = 0;
    std::string winner_name;
};

// Snapshot completo que se envía a los clientes
struct GameState {
    std::vector<InfoPlayer> players;  // todos los jugadores (cars) en la carrera
    std::vector<CheckpointInfo> checkpoints;
    TrackInfo track_info;
    RaceInfo race_info;

    GameState() = default;

    // Constructor helper: crear snapshot desde objetos Player* del servidor.
    // Se asume que la clase Player (server) provee métodos con estos nombres:
    //   getId(), getName(), getSelectedCar(), getCarType(), getCity(),
    //   getX(), getY(), getAngle(), getSpeed(), getCompletedLaps(),
    //   isDrifting(), isColliding(), isFinished(), isDisconnected()
    //
    // Si tus métodos tienen otros nombres, cambia las llamadas en la implementación .cpp
    explicit GameState(const std::vector<Player*>& current_players) {
        for (const auto& pptr : current_players) {
            if (!pptr)
                continue;
            InfoPlayer ip;
            // Estos métodos son suposiciones; adapta si tus getters se llaman distinto.
            ip.player_id = pptr->getId();
            ip.username = pptr->getName();
            ip.car_name = pptr->getSelectedCar();
            ip.car_type = pptr->getCarType();
            ip.pos_x = pptr->getX();
            ip.pos_y = pptr->getY();
            ip.angle = pptr->getAngle();
            ip.speed = pptr->getSpeed();
            ip.completed_laps = pptr->getCompletedLaps();
            ip.is_drifting = pptr->isDrifting();
            ip.is_colliding = pptr->isColliding();
            ip.race_finished = pptr->isFinished();
            ip.disconnected = pptr->isDisconnected();

            players.push_back(std::move(ip));
        }

        // Nota: checkpoints / track_info / race_info deben llenarse desde la Race/Match.
        // Aquí sólo inicializamos players. Completar estos campos desde la lógica del Race.
    }

    // Buscar jugador en snapshot por id
    InfoPlayer* findPlayer(int id) {
        for (auto& p : players) {
            // cppcheck-suppress useStlAlgorithm
            if (p.player_id == id)
                return &p;
        }
        return nullptr;
    }
};

#endif  // GAME_STATE_H_
