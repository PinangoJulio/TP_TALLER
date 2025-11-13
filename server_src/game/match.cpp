#include "match.h"
#include "race.h"  // ← IMPORTANTE: Incluir race.h, no solo forward declaration
#include <iostream>

Match::Match(std::string host_name, int code, int max_players, 
             const std::string& map_yaml_path)
    : match_code(code),
      host_name(host_name),
      is_active(true),
      queue_comandos(),
      cant_actual_players(0),
      cant_max_players(max_players),
      race_path(map_yaml_path),
      active_races(nullptr) {  // Ahora está en el orden correcto
    
    std::cout << "[Match] Match " << code << " created by " << host_name << std::endl;
}

bool Match::can_player_join_match() const {
    return cant_actual_players < cant_max_players;
}

bool Match::add_player(int id, std::string nombre, Queue<GameState>& queue_enviadora) {
    if (!can_player_join_match()) {
        return false;
    }
    
    players_queues.add_client_queue(queue_enviadora, id);
    std::cout << "[Match] Player " << id << " (" << nombre << ") joined match " 
              << match_code << std::endl;
    cant_actual_players++;
    return true;
}

bool Match::remove_player(int id_jugador) {
    players_queues.delete_client_queue(id_jugador);
    cant_actual_players--;
    std::cout << "[Match] Player " << id_jugador << " left match " << match_code << std::endl;
    return true;
}

void Match::startNextRace(const std::string& mapa_yaml_path) {
    std::cout << "[Match] Starting race on map: " << mapa_yaml_path << std::endl;
    
    // TODO: Cuando Race esté completo, descomentar:
    // active_races = std::make_unique<Race>(
    //     queue_comandos, 
    //     players_queues, 
    //     mapa_yaml_path
    // );
    // active_races->start();
}

bool Match::isRunning() const {
    return active_races && active_races->isRunning();  // ← Usar plural
}

Match::~Match() {
    is_active = false;
    if (active_races) {  // ← Usar plural
        active_races->stop();
        active_races->join();
    }
}