#ifndef GAME_STATE_H_
#define GAME_STATE_H_

#include <vector>
#include <string>
#include <cstddef>
#include <iostream>
#include "../server_src/game/player.h"

// Represents the complete state of the game at a given moment,
// which the server sends to the clients.

class Track;
class Race;


enum MatchStatus {
    WAITING_FOR_PLAYERS,
    IN_PROGRESS,
    FINISHED
};

struct CarInfo {
    int player_id;            // Player ID that owns the car
    std::string name;         // Player name
    std::string selected_car; // Player's chosen car model
    std::string city;         // City / track origin or chosen city
    float pos_x;              // Car X position
    float pos_y;              // Car Y position
    float angle;              // Car direction (angle)
    float speed;              // Current speed
    int completed_laps;       // Number of completed laps
    bool is_drifting;         // Whether the car is drifting
    bool is_colliding;        // Whether the car recently collided
    bool race_finished;       // Whether the player has finished the race
    bool disconnected;        // Whether the player has disconnected

    CarInfo()
        : player_id(0),
          selected_car(),
          city(),
          pos_x(0),
          pos_y(0),
          angle(0),
          speed(0),
          completed_laps(0),
          is_drifting(false),
          is_colliding(false),
          race_finished(false),
          disconnected(false) {}
};

// Checkpoint or intermediate goal of the track
struct CheckpointInfo {
    int id;
    float pos_x;
    float pos_y;
    bool reached;
};

// Track (map) data
struct TrackInfo {
    std::string name;
    int total_laps;
    float total_length;
};

// General race status
struct RaceInfo {
    MatchStatus status;
    int remaining_time_seconds;
    int players_finished;
    int total_players;
    std::string winner_name;
};

// This is the general "snapshot" of the game
struct GameState {
    std::vector<CarInfo> cars;               // All cars in the race
    std::vector<CheckpointInfo> checkpoints; // Active checkpoints
    TrackInfo track_info;                    // Track data
    RaceInfo race_info;                      // General race info

    GameState() = default;

    // Constructor that generates the snapshot from the server state
    // // --- Estado global de la partida (Match) ---
    GameState(std::vector<Player*>& current_players)/*
             const std::vector<Checkpoint>& current_checkpoints,
             const Track& track,
             const Race& race)*/
    {
        // Convert Player -> CarInfo
        for (const auto& player_ptr : current_players) {
            CarInfo info;
            info.player_id = player_ptr->getId();
            info.name = player_ptr->getName();
            info.selected_car = player_ptr->getSelectedCar();
            info.city = player_ptr->getCity();
            info.pos_x = player_ptr->getX();
            info.pos_y = player_ptr->getY();
            info.angle = player_ptr->getAngle();
            info.speed = player_ptr->getSpeed();
            info.completed_laps = player_ptr->getCompletedLaps();
            info.is_drifting = player_ptr->isDrifting();
            info.is_colliding = player_ptr->isColliding();
            info.race_finished = player_ptr->isFinished();
            info.disconnected = player_ptr->isDisconnected();

            cars.push_back(info);
        }

        // Convert checkpoints
        /*for (const auto& chk : current_checkpoints) {
            CheckpointInfo info;
            info.id = chk.getId();
            info.pos_x = chk.getX();
            info.pos_y = chk.getY();
            info.reached = chk.isReached();
            checkpoints.push_back(info);
        }*/

        // Track data
       /* track_info.name = track.getName();
        track_info.total_laps = track.getTotalLaps();
        track_info.total_length = track.getLength();

        // General race data
        race_info.status = race.getStatus();
        race_info.remaining_time_seconds = race.getRemainingTime();
        race_info.players_finished = race.getPlayersFinished();
        race_info.total_players = race.getTotalPlayers();
        race_info.winner_name = race.getWinner();*/
    }


    // Find a car by player ID
    CarInfo* getCarByPlayer(int id) {
        for (auto& c : cars) {
            if (c.player_id == id)
                return &c;
        }
        return nullptr;
    }
};

#endif // GAME_STATE_H_
