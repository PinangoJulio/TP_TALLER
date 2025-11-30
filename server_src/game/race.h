// filepath: /home/lou/Escritorio/Taller/TP-Final/TP_TALLER/server_src/game/race.h
#ifndef RACE_H
#define RACE_H

#include <iostream>
#include <string>

/**
 * Race: Contenedor de configuración de una carrera
 * Ya NO gestiona su propio GameLoop - solo almacena metadata
 */
class Race {
private:
    std::string city_name;
    std::string map_yaml_path;
    int total_laps;
    int race_number;  // Número de carrera en la secuencia (1, 2, 3...)

public:
    Race(const std::string& city, const std::string& yaml_path, int number = 1)
        : city_name(city), map_yaml_path(yaml_path), race_number(number) {
        std::cout << "[Race] Configuración creada: " << city_name << " - " << yaml_path;
    }

    // ---- GETTERS ----
    const std::string& get_city_name() const { return city_name; }
    const std::string& get_map_path() const { return map_yaml_path; }
    int get_total_laps() const { return total_laps; }
    int get_race_number() const { return race_number; }

    // ---- SETTERS ----
    void set_total_laps(int laps) { total_laps = laps; }
    void set_race_number(int number) { race_number = number; }

    ~Race() {
        std::cout << "[Race] Configuración destruida: " << city_name << "\n";
    }
};

#endif  // RACE_H

