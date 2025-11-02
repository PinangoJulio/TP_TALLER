#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include <string>
#include <vector>
#include <cstdint>

class GameRoom {
private:
    uint16_t game_id;
    std::string game_name;
    std::vector<std::string> players;  // Lista de usernames
    uint8_t max_players;
    bool is_started;

public:
    // Constructor
    GameRoom(uint16_t id, const std::string& name, uint8_t max_players = 8);

    // Agregar un jugador a la partida
    bool add_player(const std::string& username);

    // Verificar si la partida está llena
    bool is_full() const;

    // Verificar si la partida está lista para comenzar
    bool is_ready() const;

    // Iniciar la partida
    void start();

    // Getters
    uint16_t get_id() const { return game_id; }
    std::string get_name() const { return game_name; }
    uint8_t get_current_players() const { return players.size(); }
    uint8_t get_max_players() const { return max_players; }
    bool has_started() const { return is_started; }

    // No se pueden copiar GameRooms
    GameRoom(const GameRoom&) = delete;
    GameRoom& operator=(const GameRoom&) = delete;
};

#endif // GAME_ROOM_H
