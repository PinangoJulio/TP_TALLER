#include "match.h"

Match::Match(std::string host_name, int code, int max_players, const std::string &map_yaml_path) :
 match_code(code), host_name(host_name), is_active(true), queue_comandos(), cant_max_players(max_players), race_path(map_yaml_path){}


bool Match::add_player(int id, std::string nombre, Queue<GameState> &queue_enviadora) {
  players_queues.add_client_queue(queue_enviadora, id);
    std::cout << "Player " << id << " added to queue " << nombre << std::endl;
    cant_actual_players++;
    return true;
}

Match::~Match() {
    is_active = false;
}




