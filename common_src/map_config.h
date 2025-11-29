#ifndef MAP_CONFIG_H
#define MAP_CONFIG_H

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

// Posición de spawn para un jugador
struct SpawnPosition {
    float x;
    float y;
    float angle_degrees;  // Ángulo en grados (0 = arriba)
};

// Información de un checkpoint
struct CheckpointConfig {
    int id;
    float x;
    float y;
    float width;
    float angle;
    bool is_start;
    bool is_finish;
};

// Información de un hint/flecha
struct HintConfig {
    int id;
    float x;
    float y;
    float direction_angle;
    int for_checkpoint;
};

// Configuración completa del mapa
class MapConfig {
private:
    std::string city_name;
    std::string race_name;
    int total_laps;
    
    std::vector<SpawnPosition> spawn_positions;
    std::vector<CheckpointConfig> checkpoints;
    std::vector<HintConfig> hints;

public:
    explicit MapConfig(const std::string& yaml_path);
    
    // Getters
    const std::string& get_city() const { return city_name; }
    const std::string& get_race_name() const { return race_name; }
    int get_laps() const { return total_laps; }
    
    const std::vector<SpawnPosition>& get_spawn_positions() const { 
        return spawn_positions; 
    }
    
    const std::vector<CheckpointConfig>& get_checkpoints() const { 
        return checkpoints; 
    }
    
    const std::vector<HintConfig>& get_hints() const { 
        return hints; 
    }
    
    // Obtener spawn position para un jugador específico
    SpawnPosition get_spawn_for_player(int player_index) const {
        if (spawn_positions.empty()) {
            // Fallback: posición por defecto
            return {100.0f, 100.0f, 0.0f};
        }
        
        // Si hay más jugadores que spawns, reutilizar posiciones
        int idx = player_index % spawn_positions.size();
        return spawn_positions[idx];
    }
    
private:
    void parse_yaml(const std::string& yaml_path);
};

#endif // MAP_CONFIG_H