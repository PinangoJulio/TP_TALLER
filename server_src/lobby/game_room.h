#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

enum class RoomState : uint8_t {
    WAITING,    // Esperando jugadores
    READY,      // 2+ jugadores, puede iniciarse
    STARTED     // Juego en curso
};


struct LobbyPlayerInfo {
    bool is_host;
    bool is_ready;
    uint8_t car_index;
};

class GameRoom {
private:
    uint16_t game_id;
    std::string game_name;
    // Uso del tipo renombrado
    std::map<std::string, LobbyPlayerInfo> players;  // username -> LobbyPlayerInfo
    uint8_t max_players;
    // Uso del tipo renombrado
    RoomState state;

public:
    // Constructor
    GameRoom(uint16_t id, const std::string& name, const std::string& host, uint8_t max_players);

    // Agregar un jugador a la partida
    bool add_player(const std::string& username);

    // Remover un jugador de la partida
    void remove_player(const std::string& username);

    // Verificar si un jugador es el host
    bool is_host(const std::string& username) const;

    // Verificar si un jugador está en la partida
    bool has_player(const std::string& username) const;

    // Verificar si la partida está lista para comenzar (2+ jugadores)
    bool is_ready() const;

    // Verificar si la partida ya inició
    bool is_started() const;

    // Iniciar la partida
    void start();

    // Selección de auto
    bool set_player_car(const std::string& username, uint8_t car_index);
    uint8_t get_player_car(const std::string& username) const;
    bool all_players_selected_car() const;

    // Getters
    uint16_t get_game_id() const;
    std::string get_game_name() const;
    uint8_t get_player_count() const;
    uint8_t get_max_players() const;
    // Uso del tipo renombrado
    const std::map<std::string, LobbyPlayerInfo>& get_players() const;

    // No se pueden copiar GameRooms
    GameRoom(const GameRoom&) = delete;
    GameRoom& operator=(const GameRoom&) = delete;
};

#endif // GAME_ROOM_H
