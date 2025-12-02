#include "map_loader.h"
#include <iostream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

void MapLoader::load_map(b2WorldId world, ObstacleManager& obstacle_manager, const std::string& map_path) {
    std::cout << "[MapLoader] Loading geometry from: " << map_path << std::endl;
    
    if (!std::filesystem::exists(map_path)) {
        throw std::runtime_error("Map file not found: " + map_path);
    }
    
    YAML::Node map = YAML::LoadFile(map_path);
    
    // 1. INTENTAR CARGAR PAREDES NUEVAS (Del Script Python - walls_chain)
    if (map["walls_chain"]) {
        std::cout << "[MapLoader] ✅ Usando capa optimizada 'walls_chain'...\n";
        load_layer_chains(world, map["walls_chain"]);
    } 
    else if (map["nivel0"]) {
        // 2. FALLBACK: Si no hay walls_chain, cargamos nivel0 pero como ROAD (piso)
        // Esto evita que el auto se quede pegado al nacer.
        std::cout << "[MapLoader] ⚠️ Usando capa 'nivel0' (modo compatibilidad)...\n";
        load_layer_polygons(world, map["nivel0"], false);
    }

    // Cargar otras capas
    if (map["puentes"]) {
        load_layer_polygons(world, map["puentes"], false);
    }
    if (map["rampas"]) {
        load_layer_polygons(world, map["rampas"], true); // is_sensor = true
    }
    
    // Paredes legacy (rectángulos manuales)
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

void MapLoader::load_layer_chains(b2WorldId world, const YAML::Node& layerNode) {
    int count = 0;
    for (const auto& chain : layerNode) {
        std::vector<b2Vec2> points;
        for (const auto& point : chain) {
            float x = point[0].as<float>();
            float y = point[1].as<float>();
            points.push_back({x / PPM, y / PPM}); 
        }

        if (points.size() < 2) continue;

        while (points.size() < 4) {
            b2Vec2 pLast = points.back();
            b2Vec2 pPrev = points[points.size() - 2];
            
            b2Vec2 midPoint = { (pLast.x + pPrev.x) * 0.5f, (pLast.y + pPrev.y) * 0.5f };
            
            points.insert(points.end() - 1, midPoint);
        }

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.position = {0.0f, 0.0f}; 
        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = points.data();
        chainDef.count = points.size();
        chainDef.isLoop = false; 
        
        // Categoría WALL
        chainDef.filter.categoryBits = CATEGORY_WALL;
        chainDef.filter.maskBits = CATEGORY_CAR; 

        b2CreateChain(bodyId, &chainDef);
        count++;
    }
    std::cout << "   -> Loaded wall chains: " << count << std::endl;
}

void MapLoader::load_layer_polygons(b2WorldId world, const YAML::Node& layerNode, bool is_sensor) {
    int count = 0;
    for (const auto& polygon : layerNode) {
        std::vector<b2Vec2> points;
        for (const auto& point : polygon) {
            float x = point[0].as<float>();
            float y = point[1].as<float>();
            points.push_back({x / PPM, y / PPM}); 
        }

        if (points.size() < 3) continue;

        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {0.0f, 0.0f}; 
        b2BodyId bodyId = b2CreateBody(world, &bodyDef);

        b2ChainDef chainDef = b2DefaultChainDef();
        chainDef.points = points.data();
        chainDef.count = points.size();
        chainDef.isLoop = true; // Polígono cerrado
        
        // CONFIGURACIÓN DE FILTROS
        if (is_sensor) {
            // Rampas/Sensores
            chainDef.filter.categoryBits = CATEGORY_SENSOR;
        } else {
            // Nivel0 (Piso visual)
            // Le damos categoría ROAD. El auto IGNORA esta categoría, así que no se traba.
            chainDef.filter.categoryBits = CATEGORY_ROAD;
            chainDef.filter.maskBits = 0; 
        }
        
        b2CreateChain(bodyId, &chainDef);
        count++;
    }
    if (count > 0) std::cout << "   -> Loaded polygons: " << count << std::endl;
}

void MapLoader::load_race_config(const std::string& race_path) {
    std::cout << "[MapLoader] Loading race config from: " << race_path << std::endl;
    
    if (!std::filesystem::exists(race_path)) {
        throw std::runtime_error("Race configuration file not found: " + race_path);
    }

    YAML::Node race = YAML::LoadFile(race_path);
    
    // 1. CARGAR CHECKPOINTS
    if (race["checkpoints"]) {
        checkpoints.clear(); 
        for (const auto& cp_node : race["checkpoints"]) {
            CheckpointData cp;
            cp.id = cp_node["id"].as<int>();
            
            // Conversión: Píxeles -> Metros (Box2D)
            cp.x = cp_node["x"].as<float>();
            cp.y = cp_node["y"].as<float>();
            
            // Valores por defecto si faltan en el YAML
            cp.width = cp_node["width"] ? cp_node["width"].as<float>() : 50.0f;
            cp.height = cp_node["height"] ? cp_node["height"].as<float>() : 50.0f;
        
            
            float angle_deg = cp_node["angle"] ? cp_node["angle"].as<float>() : 0.0f;
            cp.angle = angle_deg * (M_PI / 180.0f);

            checkpoints.push_back(cp);
        }
        std::cout << "[MapLoader] Loaded " << checkpoints.size() << " checkpoints." << std::endl;
    }

    // 2. CARGAR SPAWN POINTS
    if (race["spawn_points"]) {
        spawn_points.clear();
        for (const auto& sp : race["spawn_points"]) {
            SpawnPoint s;
            
            s.x = sp["x"].as<float>();
            s.y = sp["y"].as<float>();
            
            float angle_deg = sp["angle"] ? sp["angle"].as<float>() : 0.0f;
            // Convertir grados a radianes para Box2D
            s.angle = angle_deg * (M_PI / 180.0f);
            
            std::cout << "[MapLoader]   Spawn: (" << s.x << ", " << s.y 
                      << ")px, " << angle_deg << "° -> " << s.angle << " rad\n";
            
            spawn_points.push_back(s);
        }
        std::cout << "[MapLoader] Loaded " << spawn_points.size() << " spawn points." << std::endl;
    }
}