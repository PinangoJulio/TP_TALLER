#include "map_loader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <filesystem>

void MapLoader::load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& yaml_path) {
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
        }
        std::cout << "[MapLoader] Loaded " << spawn_points.size() << " spawn points." << std::endl;
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
        }
        std::cout << "[MapLoader] Loaded " << checkpoints.size() << " checkpoints." << std::endl;
    }
    
    if (map["walls"]) {
        int count = 0;
        for (const auto& wall_node : map["walls"]) {
            float x = wall_node["x"].as<float>();
            float y = wall_node["y"].as<float>();
            float width = wall_node["width"].as<float>();
            float height = wall_node["height"].as<float>();
            
            obstacle_manager.create_wall(world, x, y, width, height); 
            count++;
        }
        std::cout << "[MapLoader] Created " << count << " walls." << std::endl;
    }
    
    if (map["buildings"]) {
        int count = 0;
        for (const auto& node : map["buildings"]) {
            float x = node["x"].as<float>();
            float y = node["y"].as<float>();
            float width = node["width"].as<float>();
            float height = node["height"].as<float>();
            
            obstacle_manager.create_building(world, x, y, width, height);
            count++;
        }
        std::cout << "[MapLoader] Created " << count << " buildings." << std::endl;
    }

    if (map["barriers"]) {
        int count = 0;
        for (const auto& node : map["barriers"]) {
            float x = node["x"].as<float>();
            float y = node["y"].as<float>();
            float length = node["length"].as<float>();
            
            obstacle_manager.create_barrier(world, x, y, length);
            count++;
        }
        std::cout << "[MapLoader] Created " << count << " barriers." << std::endl;
    }
}