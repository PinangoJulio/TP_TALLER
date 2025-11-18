#include "map_loader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <filesystem>

MapLoader::MapLoader(b2WorldId world, ObstacleManager& obstacles)
    : world(world), obstacle_manager(obstacles) {
    std::cout << "[MapLoader] Initialized" << std::endl;
}

void MapLoader::load_map(const std::string& yaml_path) {
    std::cout << "[MapLoader] Loading map: " << yaml_path << std::endl;
    
    if (!std::filesystem::exists(yaml_path)) {
        throw std::runtime_error("Map file not found: " + yaml_path);
    }
    
    YAML::Node map = YAML::LoadFile(yaml_path);
    
    if (map["spawn_points"]) {
        for (const auto& spawn_node : map["spawn_points"]) {
            SpawnPoint spawn;
            spawn.x = spawn_node["x"].as<float>();
            spawn.y = spawn_node["y"].as<float>();
            spawn.angle = spawn_node["angle"].as<float>(0.0f);
            
            spawn_points.push_back(spawn);
            std::cout << "[MapLoader] Spawn point: (" << spawn.x << ", " 
                      << spawn.y << ") angle: " << spawn.angle << std::endl;
        }
    }
    
    if (map["checkpoints"]) {
        for (const auto& cp_node : map["checkpoints"]) {
            CheckpointData cp;
            cp.id = cp_node["id"].as<int>();
            cp.x = cp_node["x"].as<float>();
            cp.y = cp_node["y"].as<float>();
            cp.width = cp_node["width"].as<float>(5.0f);
            cp.height = cp_node["height"].as<float>(1.0f);
            cp.angle = cp_node["angle"].as<float>(0.0f);
            
            checkpoints.push_back(cp);
            std::cout << "[MapLoader] Checkpoint " << cp.id << ": (" 
                      << cp.x << ", " << cp.y << ")" << std::endl;
        }
    }
    
    if (map["walls"]) {
        for (const auto& wall_node : map["walls"]) {
            float x = wall_node["x"].as<float>();
            float y = wall_node["y"].as<float>();
            float width = wall_node["width"].as<float>();
            float height = wall_node["height"].as<float>();
            
            obstacle_manager.create_wall(x, y, width, height);
            std::cout << "[MapLoader] Wall created at (" << x << ", " << y << ")" << std::endl;
        }
    }
    
    if (map["buildings"]) {
        for (const auto& building_node : map["buildings"]) {
            float x = building_node["x"].as<float>();
            float y = building_node["y"].as<float>();
            float width = building_node["width"].as<float>();
            float height = building_node["height"].as<float>();
            
            obstacle_manager.create_building(x, y, width, height);
            std::cout << "[MapLoader] Building created at (" << x << ", " << y << ")" << std::endl;
        }
    }
    
    if (map["barriers"]) {
        for (const auto& barrier_node : map["barriers"]) {
            float x = barrier_node["x"].as<float>();
            float y = barrier_node["y"].as<float>();
            float length = barrier_node["length"].as<float>();
            
            obstacle_manager.create_barrier(x, y, length);
            std::cout << "[MapLoader] Barrier created at (" << x << ", " << y << ")" << std::endl;
        }
    }
    
    std::cout << "[MapLoader] Map loaded successfully!" << std::endl;
    std::cout << "  - Spawn points: " << spawn_points.size() << std::endl;
    std::cout << "  - Checkpoints: " << checkpoints.size() << std::endl;
}

MapLoader::~MapLoader() {
    std::cout << "[MapLoader] Destroyed" << std::endl;
}
