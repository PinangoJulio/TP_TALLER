#include "game_state.h"

#include <iostream>

// Incluir las clases del servidor SOLO en este .cpp
#include "../server_src/game/car.h"
#include "../server_src/game/player.h"

// Constructor que convierte Player* a InfoPlayer
GameState::GameState(const std::vector<Player*>& player_list, const std::string& city,
                     const std::string& map_path, bool running,
                     const std::map<int, uint32_t>& current_race_times,
                     const std::map<int, uint32_t>& total_times) {
    // Llenar información de todos los jugadores
    for (const auto& player_ptr : player_list) {
        if (!player_ptr)
            continue;

        InfoPlayer info;

        // Información básica del jugador
        info.player_id = player_ptr->getId();
        info.username = player_ptr->getName();
        info.car_name = player_ptr->getSelectedCar();
        info.car_type = player_ptr->getCarType();


        // Posición y física
        info.pos_x = player_ptr->getX();
        info.pos_y = player_ptr->getY();
        info.angle = player_ptr->getAngle();
        info.speed = player_ptr->getSpeed();

        // Velocidad (del Car)
        const Car* car = player_ptr->getCar();
        if (car) {
            info.velocity_x = car->getVelocityX();
            info.velocity_y = car->getVelocityY();
            info.health = car->getHealth();
            info.nitro_amount = car->getNitroAmount();
            info.nitro_active = car->isNitroActive();
            info.is_alive = !car->isDestroyed();
        }

        // Progreso en la carrera
        info.completed_laps = player_ptr->getCompletedLaps();
        info.current_checkpoint = player_ptr->getCurrentCheckpoint();
        info.position_in_race = player_ptr->getPositionInRace();

        // Tiempos de carrera
        auto race_time_it = current_race_times.find(info.player_id);
        info.race_time_ms = (race_time_it != current_race_times.end()) ? race_time_it->second : 0;

        auto total_time_it = total_times.find(info.player_id);
        info.total_time_ms = (total_time_it != total_times.end()) ? total_time_it->second : 0;

        // Estados
        info.is_drifting = player_ptr->isDrifting();
        info.is_colliding = player_ptr->isColliding();
        info.race_finished = player_ptr->isFinished();
        info.disconnected = player_ptr->isDisconnected();

        players.push_back(info);
    }

    // Llenar race current info
    race_current_info.city = city;
    race_current_info.race_name = map_path;
    race_current_info.total_laps = 0;
    race_current_info.total_checkpoints = 0;  // Contar checkpoints del mapa

    // Llenar race info
    race_info.status = running ? MatchStatus::IN_PROGRESS : MatchStatus::WAITING_FOR_PLAYERS;
    race_info.race_number = 1;             // Obtener número de carrera actual
    race_info.total_races = 1;             // Obtener total de carreras
    race_info.remaining_time_ms = 600000;  // Calcular tiempo restante
    race_info.players_finished = 0;        //  Contar jugadores que terminaron
    race_info.total_players = static_cast<int32_t>(players.size());
    race_info.winner_name = "";  //  Determinar ganador

    // Llenar checkpoints, hints, NPCs, eventos
}
