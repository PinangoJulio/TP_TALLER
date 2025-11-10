#ifndef MATCH_H
#define MATCH_H

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>

#include "../../common_src/queue.h"
#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"
#include "../network/client_monitor.h" // El Broadcaster
#include "race.h"

class Race;

class Match {
private:
    int match_code;
    std::string host_name;
    std::atomic<bool> is_active;
    Queue<ComandMatchDTO> queue_comandos;

    // LISTA DE QUEUES (BROADCASTER): Contiene las colas de salida de TODOS los jugadores conectados a esta Partida. Permite enviar Snapshots.
    ClientMonitor players_queues;

    // Instancia de la carrera actual (usa unique_ptr para gestionar la vida de la simulación)
    //std::unique_ptr<Race> active_races;

    int cant_actual_players;
    int cant_max_players;
    std::string race_path;

    // (Aca iría la tabla de posiciones ACUMULADA de la Partida)

public:
    Match(std::string host_name,
            int code,
            int max_players,
            const std::string& map_yaml_path);

    // MÉTODOS DE LOBBY/UNIÓN
    bool can_player_join_match() const;
    bool add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora);
    bool remove_player(int id_jugador);

    // MÉTODOS DE CONTROL DE CARRERA
    void startNextRace(const std::string& mapa_yaml_path); // Inicia un nuevo hilo de Carrera
    bool isRunning() const; // Devuelve el estado de la Carrera activa

    // GETTERS
    std::string get_host_name() {return host_name;}
    int getMatchCode() const { return this->match_code; }
    Queue<ComandMatchDTO>& getComandQueue() { return this->queue_comandos; }

    ~Match();
};

#endif //MATCH_H