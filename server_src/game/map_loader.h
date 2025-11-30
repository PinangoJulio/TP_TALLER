#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <box2d/box2d.h>
#include <yaml-cpp/yaml.h>
#include "obstacle.h"
#include "checkpoint_manager.h"

// Factor de conversión: 30 píxeles = 1 metro
// Ajusta esto si tus autos quedan muy grandes o chicos
const float PPM = 30.0f; 

struct SpawnPoint {
    float x;
    float y;
    float angle;  
};

class MapLoader {
private:
    std::vector<SpawnPoint> spawn_points;
    std::vector<CheckpointData> checkpoints;

    void load_layer_polygons(b2WorldId world, const YAML::Node& layerNode, bool is_sensor);

public:
    MapLoader() = default;
    
    void load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& map_path);
    
    void load_race_config(const std::string& race_path);
    
    const std::vector<SpawnPoint>& get_spawn_points() const { return spawn_points; }
    const std::vector<CheckpointData>& get_checkpoints() const { return checkpoints; }
};

#endif