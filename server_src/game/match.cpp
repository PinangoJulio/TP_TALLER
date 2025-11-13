#include "match.h"

#include "common_src/config.h"

Match::Match(std::string host_name, int code, int max_players)
        : host_name(std::move(host_name)),
          match_code(code),
          is_active(false),
          current_race_index(0),
          players_queues(),
          command_queue(),
          max_players(max_players){}

bool Match::can_player_join_match() const {
    return static_cast<int>(players.size()) < max_players;
}


bool Match::add_player(int id, std::string nombre, Queue<GameState> &queue_enviadora) {
    if (!can_player_join_match()) return false;
    auto player = std::make_unique<Player>(id, nombre);
    players_queues.add_client_queue(queue_enviadora, id); // para enviar updates
    players.push_back(std::move(player));

    std::cout << "[Match] Jugador agregado: " << nombre << " (id=" << id << ")\n";
    return true;
}

bool Match::remove_player(int id_jugador) {
    auto it = std::remove_if(players.begin(), players.end(),
                             [id_jugador](const std::unique_ptr<Player>& p){ return p->getId() == id_jugador; });

    if (it == players.end()) return false;

    players.erase(it, players.end());
    players_queues.delete_client_queue(id_jugador);

    std::cout << "[Match] Jugador eliminado: id=" << id_jugador << "\n";
    return true;
}

void Match::set_car(int player_id, const std::string& car_name, const std::string& car_type) {

    for (auto& player : players) {
        if (player->getId() == player_id) {
            player->setSelectedCar(car_name);
            player->getCarType(car_type); // ojo, parece mal nombrado (debería ser setCarType)
            std::cout << "[Match] Jugador " << player->getName()
                      << " eligió auto " << car_name << " tipo " << car_type << "\n";
            // habria que agregar campo Car al Player y cargarle toda la info inicial (speed, accel, etc)
            break;
        }
    }
}

void Match::add_race(const std::string& yaml_path, const std::string& city_name) {
    races.push_back(std::make_unique<Race>(command_queue, players_queues, city_name, yaml_path));
    std::cout << "[Match] Carrera agregada: " << city_name << " -> " << yaml_path << "\n";
}


void Match::start_next_race() {
    if (races.empty()) {
        std::cerr << "[Match] No hay carreras configuradas.\n";
        return;
    }
    if (current_race_index >= static_cast<int>(races.size())) {
        std::cout << "[Match] Todas las carreras finalizadas.\n";
        return;
    }

    is_active.store(true);
    std::string yaml = races[current_race_index]->get_map_path();
    std::cout << "[Match] Iniciando carrera #" << current_race_index + 1
              << " en mapa " << yaml << "\n";

    races[current_race_index]->start();
    current_race_index++;
}

Match::~Match() {
    is_active = false;
    races.clear();
}




