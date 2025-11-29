#include "map_config.h"
#include <iostream>
#include <stdexcept>

MapConfig::MapConfig(const std::string& yaml_path) {
    parse_yaml(yaml_path);
}

void MapConfig::parse_yaml(const std::string& yaml_path) {
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        
        // 1. Leer información básica de la carrera
        if (config["race_info"]) {
            auto race_info = config["race_info"];
            city_name = race_info["city"].as<std::string>();
            race_name = race_info["name"].as<std::string>();
            total_laps = race_info["laps"].as<int>(3); // Default: 3 laps
        } else {
            throw std::runtime_error("Missing 'race_info' section in YAML");
        }
        
        // 2. Leer posiciones de spawn
        if (config["spawn_positions"]) {
            for (const auto& spawn_node : config["spawn_positions"]) {
                SpawnPosition spawn;
                spawn.x = spawn_node[0].as<float>();
                spawn.y = spawn_node[1].as<float>();
                spawn.angle_degrees = spawn_node[2].as<float>();
                spawn_positions.push_back(spawn);
            }
            
            std::cout << "[MapConfig] Loaded " << spawn_positions.size() 
                      << " spawn positions" << std::endl;
        } else {
            std::cerr << "[MapConfig] ⚠️ No spawn_positions found, using defaults" 
                      << std::endl;
            // Agregar una posición por defecto
            spawn_positions.push_back({100.0f, 100.0f, 0.0f});
        }
        
        // 3. Leer checkpoints
        if (config["checkpoints"]) {
            for (const auto& cp_node : config["checkpoints"]) {
                CheckpointConfig cp;
                cp.id = cp_node["id"].as<int>();
                cp.x = cp_node["x"].as<float>();
                cp.y = cp_node["y"].as<float>();
                cp.width = cp_node["width"].as<float>(100.0f);
                cp.angle = cp_node["angle"].as<float>(0.0f);
                cp.is_start = cp_node["is_start"].as<bool>(false);
                cp.is_finish = cp_node["is_finish"].as<bool>(false);
                checkpoints.push_back(cp);
            }
            
            std::cout << "[MapConfig] Loaded " << checkpoints.size() 
                      << " checkpoints" << std::endl;
        }
        
        // 4. Leer hints
        if (config["hints"]) {
            for (const auto& hint_node : config["hints"]) {
                HintConfig hint;
                hint.id = hint_node["id"].as<int>();
                hint.x = hint_node["x"].as<float>();
                hint.y = hint_node["y"].as<float>();
                hint.direction_angle = hint_node["direction_angle"].as<float>();
                hint.for_checkpoint = hint_node["for_checkpoint"].as<int>();
                hints.push_back(hint);
            }
            
            std::cout << "[MapConfig] Loaded " << hints.size() 
                      << " hints" << std::endl;
        }
        
        std::cout << "[MapConfig] ✅ Map config loaded: " << city_name 
                  << " - " << race_name << std::endl;
        
    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Error parsing YAML file '" + yaml_path + 
                               "': " + std::string(e.what()));
    }
}