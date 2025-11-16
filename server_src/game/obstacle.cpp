#include "obstacle.h"
#include <iostream>

ObstacleManager::ObstacleManager(b2WorldId world_id) 
    : world(world_id) {
    std::cout << "[ObstacleManager] Initialized" << std::endl;
}

void ObstacleManager::create_wall(float x, float y, float width, float height) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {x, y};
    
    b2BodyId body = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(width / 2.0f, height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    obstacles.emplace_back(ObstacleType::WALL, body, 1.5f);
    
    std::cout << "[ObstacleManager] Wall created at (" << x << ", " << y 
              << ") size: " << width << "x" << height << std::endl;
}

void ObstacleManager::create_barrier(float x, float y, float length) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {x, y};
    
    b2BodyId body = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(length / 2.0f, 0.5f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    obstacles.emplace_back(ObstacleType::BARRIER, body, 1.0f);
    
    std::cout << "[ObstacleManager] Barrier created at (" << x << ", " << y 
              << ") length: " << length << std::endl;
}

void ObstacleManager::create_building(float x, float y, float width, float height) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_staticBody;
    bodyDef.position = {x, y};
    
    b2BodyId body = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(width / 2.0f, height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    
    b2CreatePolygonShape(body, &shapeDef, &box);
    
    obstacles.emplace_back(ObstacleType::WALL, body, 2.0f);
    
    std::cout << "[ObstacleManager] Building created at (" << x << ", " << y 
              << ") size: " << width << "x" << height << std::endl;
}

bool ObstacleManager::is_obstacle(b2BodyId body) const {
    for (const auto& obstacle : obstacles) {
        if (obstacle.body.index1 == body.index1 && 
            obstacle.body.world0 == body.world0) {
            return true;
        }
    }
    return false;
}

float ObstacleManager::get_damage_multiplier(b2BodyId body) const {
    for (const auto& obstacle : obstacles) {
        if (obstacle.body.index1 == body.index1 && 
            obstacle.body.world0 == body.world0) {
            return obstacle.damage_multiplier;
        }
    }
    return 1.0f;
}

void ObstacleManager::clear() {
    if (B2_IS_NON_NULL(world) && b2World_IsValid(world)) {
        for (auto& obstacle : obstacles) {
            if (B2_IS_NON_NULL(obstacle.body) && b2Body_IsValid(obstacle.body)) {
                b2DestroyBody(obstacle.body);
            }
        }
        std::cout << "[ObstacleManager] All obstacles cleared" << std::endl;
    } else {
        std::cout << "[ObstacleManager] World already destroyed, skipping body cleanup" << std::endl;
    }
    obstacles.clear();
}

ObstacleManager::~ObstacleManager() {
    clear();
}
