#ifndef RACE_H
#define RACE_H

#include <iostream>
#include <string>

/**
 * Race: Contenedor de configuración de una carrera.
 * En esta arquitectura unificada, Race NO tiene lógica activa. 
 * Solo guarda qué mapa y qué configuración usar.
 * La lógica real vive en el GameLoop central.
 */
class Race {
private:
    std::string city_name;
    std::string map_yaml_path;   // Mapa base (Paredes)
    std::string race_yaml_path;  // Configuración (Checkpoints)
    int race_number;             // Orden en el torneo (1, 2, 3...)
    int total_laps;

public:
    // Constructor actualizado: Recibe ambas rutas
    Race(const std::string& city, 
         const std::string& map_path, 
         const std::string& race_path)
        : city_name(city), 
          map_yaml_path(map_path), 
          race_yaml_path(race_path), 
          race_number(1),
          total_laps(3) {
        
        std::cout << "[Race] Config creada: " << city << "\n"
                  << "       Base: " << map_path << "\n"
                  << "       Race: " << race_path << "\n";
    }

    // ---- GETTERS ----
    const std::string& get_city_name() const { return city_name; }
    const std::string& get_map_path() const { return map_yaml_path; }
    const std::string& get_race_path() const { return race_yaml_path; }
    int get_race_number() const { return race_number; }
    int get_total_laps() const { return total_laps; }

    // ---- SETTERS ----
    void set_total_laps(int laps) { total_laps = laps; }
    void set_race_number(int number) { race_number = number; }

    ~Race() = default;
};

#endif  // RACE_H