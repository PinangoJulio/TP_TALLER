#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <box2d/box2d.h>
#include <yaml-cpp/yaml.h>
#include "obstacle_manager.h"
#include "checkpoints.h" 

// 30 píxeles = 1 metro
const float PPM = 30.0f; 

// Categorías de Colisión
static const uint16_t CATEGORY_CAR    = 0x0001;
static const uint16_t CATEGORY_ROAD   = 0x0002;
static const uint16_t CATEGORY_WALL   = 0x0004;
static const uint16_t CATEGORY_SENSOR = 0x0008;

class MapLoader {
private:
    std::vector<SpawnPoint> spawn_points;
    std::vector<Checkpoint> checkpoints;

    // Métodos privados internos
    void load_layer_polygons(b2WorldId world, const YAML::Node& layerNode, bool is_sensor);
    void load_layer_chains(b2WorldId world, const YAML::Node& layerNode);

public:
    MapLoader() = default;
    
    // API Pública
    void load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& map_path);
    void load_race_config(const std::string& race_path);
    
    // Getters
    const std::vector<SpawnPoint>& get_spawn_points() const { return spawn_points; }
    const std::vector<Checkpoint>& get_checkpoints() const { return checkpoints; }
};

#endif // MAP_LOADER_H