#ifndef MATCH_H
#define MATCH_H

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "game_loop.h"
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../../common_src/queue.h"
#include "../network/client_monitor.h"
#include "race.h"

class Race;

// Información de lobby de cada jugador
struct PlayerLobbyInfo {
    int id;
    std::string name;
    std::string car_name;
    std::string car_type;
    bool is_ready;
    Queue<GameState>* sender_queue;
};

enum class MatchState : uint8_t {
    WAITING,  // Esperando jugadores
    READY,    // >= 2 jugadores, puede iniciarse
    STARTED   // Juego en curso
};

class Match {
private:
    std::string host_name;
    int match_code;
    std::atomic<bool> is_active;
    MatchState state;

    // Configuraciones crudas
    std::vector<ServerRaceConfig> race_configs;  

    // Lista de objetos Race (Configuración de cada carrera)
    std::vector<std::unique_ptr<Race>> races; 
    int current_race_index;

    // ÚNICO GameLoop que gestiona la física y lógica
    std::unique_ptr<GameLoop> gameloop;

    ClientMonitor players_queues;
    Queue<ComandMatchDTO> command_queue;
    int max_players;

    std::map<int, PlayerLobbyInfo> players_info;
    std::map<std::string, int> player_name_to_id;
    
    // Nota: Los punteros Player reales ahora viven dentro del GameLoop, 
    // aquí mantenemos la info de lobby.

    std::mutex mtx;
    std::function<void(const std::vector<uint8_t>&, int exclude_player_id)> broadcast_callback;

    // Métodos internos
    void send_race_info_to_all_players();
    void send_game_started_confirmation();
    void start_next_race();

public:
    Match(std::string host_name, int code, int max_players);

    // ---- LOBBY: Gestión de jugadores ----
    bool can_player_join_match() const;
    bool add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora);
    bool remove_player(int id_jugador);
    bool has_player(int player_id) const;
    bool has_player_by_name(const std::string& name) const;
    int get_player_id_by_name(const std::string& name) const;

    // ---- LOBBY: Selección de auto ----
    bool set_player_car(int player_id, const std::string& car_name, const std::string& car_type);
    bool set_player_car_by_name(const std::string& player_name, const std::string& car_name,
                                const std::string& car_type);
    std::string get_player_car(int player_id) const;
    bool all_players_selected_car() const;

    // ---- LOBBY: Estado Ready ----
    bool set_player_ready(int player_id, bool ready);
    bool set_player_ready_by_name(const std::string& player_name, bool ready);
    bool is_player_ready(int player_id) const;
    bool all_players_ready() const;

    // ---- LOBBY: Snapshot ----
    std::map<int, PlayerLobbyInfo> get_players_snapshot() const;
    const std::map<int, PlayerLobbyInfo>& get_players() const; 

    // ---- LOBBY: Carreras ----
    // Método unificado que pide las 3 cosas necesarias para Box2D + Lobby
    void add_race(const std::string& map_path, const std::string& race_path, const std::string& city_name);
    
    void set_race_configs(const std::vector<ServerRaceConfig>& configs);
    const std::vector<ServerRaceConfig>& get_race_configs() const { return race_configs; }

    // ---- CARRERAS ----
    void start_match();  
    void stop_match();   
    
    bool is_running() const { return is_active.load(); }
    bool is_started() const { return state == MatchState::STARTED; }
    bool can_start() const;

    // ---- BROADCAST ----
    void set_broadcast_callback(
        std::function<void(const std::vector<uint8_t>&, int exclude_player_id)> callback) {
        broadcast_callback = callback;
    }

    const std::function<void(const std::vector<uint8_t>&, int exclude_player_id)>&
    get_broadcast_callback() const {
        return broadcast_callback;
    }

    // ---- GETTERS ----
    std::string get_host_name() const { return host_name; }
    std::string get_match_name() const { return host_name; } 
    int getMatchCode() const { return match_code; }
    int get_player_count() const;
    int get_max_players() const { return max_players; }
    bool is_empty() const;
    Queue<ComandMatchDTO>& getComandQueue() { return command_queue; }

    // Compatibility aliases
    void set_car(int player_id, const std::string& car_name, const std::string& car_type) {
        set_player_car(player_id, car_name, car_type);
    }

    // ---- DEBUG ----
    void print_players_info() const;

    ~Match();
};

#endif  // MATCH_H