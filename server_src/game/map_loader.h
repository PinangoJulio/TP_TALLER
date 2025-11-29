#ifndef MAP_LOADER_H
#define MAP_LOADER_H

#include <string>
#include <vector>
#include <box2d/box2d.h>
#include "obstacle.h"
#include "../../common_src/config.h"

struct SpawnPoint {
    float x;
    float y;
    float angle;  
};

struct CheckpointData {
    int id;
    float x;
    float y;
    float width;
    float height;
    float angle;
};

class MapLoader {
private:
    b2WorldId world;
    ObstacleManager& obstacle_manager;
    
    std::vector<SpawnPoint> spawn_points;
    std::vector<CheckpointData> checkpoints;

public:
    MapLoader(b2WorldId world, ObstacleManager& obstacles);
    
    void load_map(const std::string& yaml_path);
    
    const std::vector<SpawnPoint>& get_spawn_points() const { return spawn_points; }
    const std::vector<CheckpointData>& get_checkpoints() const { return checkpoints; }
    
    ~MapLoader();
};

#endif
