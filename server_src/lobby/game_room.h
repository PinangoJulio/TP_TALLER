#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>

enum class RoomState : uint8_t {
    WAITING,    // Esperando jugadores (< 2)
    READY,      // >= 2 jugadores, puede iniciarse
    STARTED     // Juego en curso
};

struct LobbyPlayerInfo {
    bool is_ready;
    std::string car_name;
    std::string car_type;
    // 🔥 ELIMINADO: bool is_host;
};

class GameRoom {
private:
    uint16_t game_id;
    std::string game_name;
    std::string creator_username;  // 🔥 NUEVO: Solo para info, sin privilegios
    std::map<std::string, LobbyPlayerInfo> players;
    uint8_t max_players;
    RoomState state;
    
    std::function<void(const std::vector<uint8_t>&)> broadcast_callback;

public:
    GameRoom(uint16_t id, const std::string& name, const std::string& creator, uint8_t max_players);

    void set_broadcast_callback(std::function<void(const std::vector<uint8_t>&)> callback) {
        broadcast_callback = callback;
    }
    
    const std::function<void(const std::vector<uint8_t>&)>& get_broadcast_callback() const {
        return broadcast_callback;
    }

    // Gestión de jugadores
    bool add_player(const std::string& username);
    void remove_player(const std::string& username);
    bool has_player(const std::string& username) const;
    
    // Estado de la sala
    bool is_ready() const;
    bool is_started() const;
    void start();

    // 🔥 ELIMINADO: bool is_host(const std::string& username) const;
    
    // 🔥 NUEVO: Cualquiera puede iniciar si todos están listos
    bool can_start() const { return is_ready() && all_players_ready(); }

    // Ready state
    bool set_player_ready(const std::string& username, bool ready);
    bool is_player_ready(const std::string& username) const;
    bool all_players_ready() const;

    // Auto selection
    bool set_player_car(const std::string& username, const std::string& car_name, const std::string& car_type);
    std::string get_player_car(const std::string& username) const;
    bool all_players_selected_car() const;

    // Getters
    uint16_t get_game_id() const;
    std::string get_game_name() const;
    std::string get_creator() const { return creator_username; }  // 🔥 NUEVO
    uint8_t get_player_count() const;
    uint8_t get_max_players() const;
    const std::map<std::string, LobbyPlayerInfo>& get_players() const;

    GameRoom(const GameRoom&) = delete;
    GameRoom& operator=(const GameRoom&) = delete;
};

#endif // GAME_ROOM_H