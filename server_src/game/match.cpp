#include "match.h"

Match::Match(std::string host_name, int code, int max_players, 
             const std::string &map_yaml_path, Configuracion& cfg)
    : match_code(code),
      host_name(host_name),
      is_active(true),
      queue_comandos(),
      cant_actual_players(0),
      cant_max_players(max_players),
      race_path(map_yaml_path),
      config(cfg) {
    
    std::cout << "[Match] Match " << code << " created by " << host_name << std::endl;
}

bool Match::add_player(int id, std::string nombre, Queue<GameState> &queue_enviadora) {
    players_queues.add_client_queue(queue_enviadora, id);
    std::cout << "[Match] Player " << id << " (" << nombre << ") joined match " 
              << match_code << std::endl;
    cant_actual_players++;
    return true;
}

void Match::startNextRace(const std::string& mapa_yaml_path) {
    std::cout << "[Match] Starting race on map: " << mapa_yaml_path << std::endl;
    
    active_race = std::make_unique<Race>(
        queue_comandos, 
        players_queues, 
        mapa_yaml_path, 
        config
    );
    
    // active_race->start();  //cuando Race esté completo
}

bool Match::isRunning() const {
    return active_race && active_race->isRunning();
}

Match::~Match() {
    is_active = false;
    if (active_race) {
        active_race->stop();
        active_race->join();
    }
}




