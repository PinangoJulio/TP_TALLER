#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include <string>
#include <vector>
#include <cstdint>

enum class GameState : uint8_t {
    WAITING,    // Esperando jugadores
    READY,      // 2+ jugadores, puede iniciarse
    STARTED     // Juego en curso
};

class GameRoom {
private:
    uint16_t game_id;
    std::string game_name;
    std::vector<std::string> players;  // Lista de usernames
    std::string host_username;         // NUEVO: Host de la partida
    uint8_t max_players;
    GameState state;                   // NUEVO: Estado de la partida

    // Promover al siguiente jugador como host
    void promote_new_host();

public:
    // Constructor
    GameRoom(uint16_t id, const std::string& name, const std::string& creator, uint8_t max_players = 8);

    // Agregar un jugador a la partida
    bool add_player(const std::string& username);

    // Remover un jugador de la partida
    bool remove_player(const std::string& username);

    // Verificar si un jugador es el host
    bool is_host(const std::string& username) const;

    // Verificar si un jugador está en la partida
    bool has_player(const std::string& username) const;

    // Verificar si la partida está llena
    bool is_full() const;

    // Verificar si la partida está lista para comenzar (2+ jugadores)
    bool is_ready() const;

    // Iniciar la partida (solo el host puede hacerlo)
    bool start(const std::string& requester);

    // Expulsar a un jugador (solo el host puede hacerlo)
    bool kick_player(const std::string& host, const std::string& target);

    // Obtener lista de jugadores
    const std::vector<std::string>& get_players() const { return players; }

    // Getters
    uint16_t get_id() const { return game_id; }
    std::string get_name() const { return game_name; }
    std::string get_host() const { return host_username; }
    uint8_t get_current_players() const { return players.size(); }
    uint8_t get_max_players() const { return max_players; }
    GameState get_state() const { return state; }
    bool has_started() const { return state == GameState::STARTED; }

    // No se pueden copiar GameRooms
    GameRoom(const GameRoom&) = delete;
    GameRoom& operator=(const GameRoom&) = delete;
};

#endif // GAME_ROOM_H
