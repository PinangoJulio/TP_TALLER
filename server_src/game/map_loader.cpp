#include "map_loader.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <filesystem>

void MapLoader::load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& map_path) {
    std::cout << "[MapLoader] Loading geometry from: " << map_path << std::endl;
    
    if (!std::filesystem::exists(map_path)) {
        throw std::runtime_error("Map file not found: " + map_path);
    }
    
    YAML::Node map = YAML::LoadFile(map_path);
    
    // 1. Cargar Capas de Polígonos (Nivel 0, Puentes, Rampas)
    if (map["nivel0"]) {
        load_layer_polygons(world, map["nivel0"], false);
    }
    if (map["puentes"]) {
        load_layer_polygons(world, map["puentes"], false);
    }
    if (map["rampas"]) {
        // Las rampas se cargan como sensores (aunque por ahora la lógica física sea igual)
        load_layer_polygons(world, map["rampas"], true); 
    }
    
    // Compatibilidad con formato viejo de obstáculos
    if (map["walls"]) {
        for (const auto& wall : map["walls"]) {
            obstacle_manager.create_wall(world, 
                wall["x"].as<float>() / PPM, 
                wall["y"].as<float>() / PPM, 
                wall["width"].as<float>() / PPM, 
                wall["height"].as<float>() / PPM
            );
        }
    }
}

void MapLoader::load_race_config(const std::string& race_path) {
    std::cout << "[MapLoader] Loading race config from: " << race_path << std::endl;
    
    if (!std::filesystem::exists(race_path)) {
        throw std::runtime_error("Race file not found: " + race_path);
    }

    YAML::Node race = YAML::LoadFile(race_path);
    
    // Cargar Checkpoints
    if (race["checkpoints"]) {
        checkpoints.clear(); 
        for (const auto& cp_node : race["checkpoints"]) {
            CheckpointData cp;
            cp.id = cp_node["id"].as<int>();
            // Convertir píxeles a metros
            cp.x = cp_node["x"].as<float>() / PPM;
            cp.y = cp_node["y"].as<float>() / PPM;
            
            // Usar valores por defecto si no están en el YAML
            float w = cp_node["width"] ? cp_node["width"].as<float>() : 100.0f;
            float h = cp_node["height"] ? cp_node["height"].as<float>() : 100.0f;
            
            cp.width = w / PPM;
            cp.height = h / PPM;
            cp.angle = cp_node["angle"] ? cp_node["angle"].as<float>() : 0.0f;
            
            checkpoints.push_back(cp);
        }
        std::cout << "[MapLoader] Loaded " << checkpoints.size() << " checkpoints." << std::endl;
    }

    // Cargar Spawn Points
    if (race["spawn_points"]) {
        spawn_points.clear();
        for (const auto& sp : race["spawn_points"]) {
            SpawnPoint s;
            s.x = sp["x"].as<float>() / PPM;
            s.y = sp["y"].as<float>() / PPM;
            s.angle = sp["angle"] ? sp["angle"].as<float>() : 0.0f;
            spawn_points.push_back(s);
        }
    }
}

void MapLoader::load_layer_polygons(b2WorldId world, const YAML::Node& layerNode, bool is_sensor) {
    // CORRECCIÓN: Silenciamos el warning de variable no usada
    (void)is_sensor; 

    int count = 0;
    for (const auto& polygon : layerNode) {
        std::vector<b2Vec2> points;
        
        // Cada 'polygon' es una lista de pares [x, y]
        for (const auto& point : polygon) {
            float x = point[0].as<float>();
            float y = point[1].as<float>();
            
            // Convertir a metros
            points.push_back({x / PPM, y / PPM}); 
        }

        if (points.size() < 3) continue; // Necesitamos al menos un triángulo

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.position = {0.0f, 0.0f}; 
        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = points.data();
        chainDef.count = points.size();
        chainDef.isLoop = true; 
        // Nota: En Box2D v3 las Chains no tienen propiedad isSensor directa en el Def,
        // se manejan con filtros o shapes hijos si se requiere.
        
        b2CreateChain(bodyId, &chainDef);
        
        count++;
    }
    std::cout << "   -> Loaded layer polygons: " << count << std::endl;
}