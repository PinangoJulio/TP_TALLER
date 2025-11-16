#ifndef GAME_ROOM_H
#define GAME_ROOM_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <functional>

enum class RoomState : uint8_t {
    WAITING,    // Esperando jugadores
    READY,      // 2+ jugadores, puede iniciarse
    STARTED     // Juego en curso
};

struct LobbyPlayerInfo {
    bool is_host;
    bool is_ready;      // ✅ NUEVO: Estado ready
    std::string car_name;
    std::string car_type;
};

class GameRoom {
private:
    uint16_t game_id;
    std::string game_name;
    std::map<std::string, LobbyPlayerInfo> players;  // username -> info
    uint8_t max_players;
    RoomState state;
    
    // 🔥 NUEVO: Callback para notificar cambios
    std::function<void(const std::vector<uint8_t>&)> broadcast_callback;

public:
    GameRoom(uint16_t id, const std::string& name, const std::string& host, uint8_t max_players);

    // 🔥 NUEVO: Registrar función de broadcast
    void set_broadcast_callback(std::function<void(const std::vector<uint8_t>&)> callback) {
        broadcast_callback = callback;
    }
    
    // 🔥 NUEVO: Obtener callback (para uso interno)
    const std::function<void(const std::vector<uint8_t>&)>& get_broadcast_callback() const {
        return broadcast_callback;
    }

    bool add_player(const std::string& username);
    void remove_player(const std::string& username);
    bool is_host(const std::string& username) const;
    bool has_player(const std::string& username) const;
    bool is_ready() const;
    bool is_started() const;
    void start();

    // 🔥 NUEVO: Manejo de ready state
    bool set_player_ready(const std::string& username, bool ready);
    bool is_player_ready(const std::string& username) const;
    bool all_players_ready() const;

    // Auto selection (ya existente)
    bool set_player_car(const std::string& username, const std::string& car_name, const std::string& car_type);
    std::string get_player_car(const std::string& username) const;
    bool all_players_selected_car() const;

    // Getters
    uint16_t get_game_id() const;
    std::string get_game_name() const;
    uint8_t get_player_count() const;
    uint8_t get_max_players() const;
    const std::map<std::string, LobbyPlayerInfo>& get_players() const;

    GameRoom(const GameRoom&) = delete;
    GameRoom& operator=(const GameRoom&) = delete;
};

#endif // GAME_ROOM_H