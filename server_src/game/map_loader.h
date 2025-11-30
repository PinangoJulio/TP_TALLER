#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <box2d/box2d.h>
#include "obstacle.h"
#include "checkpoint_manager.h" 

struct SpawnPoint {
    float x;
    float y;
    float angle;  
};

class MapLoader {
private:
    std::vector<SpawnPoint> spawn_points;
    std::vector<CheckpointData> checkpoints;

public:
    MapLoader() = default;
    
    void load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& yaml_path);
    
    const std::vector<SpawnPoint>& get_spawn_points() const { return spawn_points; }
    const std::vector<CheckpointData>& get_checkpoints() const { return checkpoints; }
};

#endif