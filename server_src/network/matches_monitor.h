#ifndef MATCHES_MONITOR_H
#define MATCHES_MONITOR_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "common_src/game_state.h"
#include "common_src/lobby_protocol.h"
#include "common_src/queue.h"
#include "common_src/socket.h"
#include "server_src/game/match.h"

class MatchesMonitor {
private:
    int id_matches = 0;
    std::mutex mtx;
    std::map<int, std::unique_ptr<Match>> matches;

    // ✅ Gestión de sockets por partida (match_id → {player_name → Socket*})
    std::map<int, std::map<std::string, Socket*>> player_sockets;

    // ✅ Lookup inverso: player_name → match_id
    std::map<std::string, int> player_to_match;

public:
    // **NUEVO: Constructor por defecto explícitamente pedido (FIX)**
    MatchesMonitor() = default;

    // Constructor de movimiento
    MatchesMonitor(MatchesMonitor&& other) = default;
    MatchesMonitor& operator=(MatchesMonitor&& other) = default;

    // Eliminación de copia
    MatchesMonitor(const MatchesMonitor& other) = delete;
    MatchesMonitor& operator=(const MatchesMonitor& other) = delete;

    // ---- LOBBY: Gestión de partidas ----
    int create_match(int max_players, const std::string& host_name, int player_id,
                     Queue<GameState>& sender_message_queue);
    bool join_match(int match_id, const std::string& player_name, int player_id,
                    Queue<GameState>& sender_message_queue);
    bool leave_match(const std::string& player_name);
    bool leave_match_by_id(int player_id, int match_id);

    bool add_races_to_match(int match_id, const std::vector<ServerRaceConfig>& race_paths);
    std::vector<GameInfo> list_available_matches();
    std::vector<std::string> get_race_paths(int match_id) const;

    // ---- LOBBY: Validaciones ----
    bool is_player_in_match(const std::string& player_name) const;
    int get_player_match(const std::string& player_name) const;
    bool is_match_ready(int match_id) const;

    // ---- LOBBY: Acciones de jugadores ----
    bool set_player_car(const std::string& player_name, const std::string& car_name,
                        const std::string& car_type);
    bool set_player_ready(const std::string& player_name, bool ready);

    // ---- LOBBY: Snapshot ----
    std::map<int, PlayerLobbyInfo> get_match_players_snapshot(int match_id) const;

    // ---- LOBBY: Sockets y Broadcast ----
    void register_player_socket(int match_id, const std::string& player_name, Socket& socket);
    void unregister_player_socket(int match_id, const std::string& player_name);
    void broadcast_to_match(int match_id, const std::vector<uint8_t>& buffer,
                            const std::string& exclude_player = "");

    // ---- GAME: Inicio de partida ----
    bool start_match(int match_id);
    Queue<ComandMatchDTO>* get_command_queue(int match_id);

    // ---- ADMIN ----
    void clear_all_matches();
    const std::map<int, std::unique_ptr<Match>>& get_all_matches() const { return matches; }
    std::string get_match_name(int match_id) const;

    // Alias de compatibilidad
    void set_player_car(int player_id, const std::string& car_name, const std::string& car_type);
    void delete_player_from_match(int player_id, int match_id);
};

#endif  // MATCHES_MONITOR_H
