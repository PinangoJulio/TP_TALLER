#include "obstacle.h"
#include <iostream>

void ObstacleManager::create_wall(b2WorldId world, float x, float y, float width, float height) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = {x, y};
    bodyDef.type = b2_staticBody; // Paredes son estáticas
    
    b2BodyId bodyId = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(width / 2.0f, height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    
    obstacles.push_back(std::make_unique<Obstacle>(bodyId, 1.0f, true));
}

void ObstacleManager::create_building(b2WorldId world, float x, float y, float width, float height) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = {x, y};
    bodyDef.type = b2_staticBody;
    
    b2BodyId bodyId = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(width / 2.0f, height / 2.0f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    
    obstacles.push_back(std::make_unique<Obstacle>(bodyId, 2.0f, true));
}

void ObstacleManager::create_barrier(b2WorldId world, float x, float y, float length) {
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = {x, y};
    bodyDef.type = b2_staticBody; 
    
    b2BodyId bodyId = b2CreateBody(world, &bodyDef);
    
    b2Polygon box = b2MakeBox(length / 2.0f, 0.5f); 
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    
    b2CreatePolygonShape(bodyId, &shapeDef, &box);
    
    obstacles.push_back(std::make_unique<Obstacle>(bodyId, 0.5f, true));
}

bool ObstacleManager::is_obstacle(b2BodyId bodyId) const {
    for (const auto& obs : obstacles) {
        if (B2_ID_EQUALS(obs->body, bodyId)) {
            return true;
        }
    }
    return false;
}

float ObstacleManager::get_damage_multiplier(b2BodyId bodyId) const {
    for (const auto& obs : obstacles) {
        if (B2_ID_EQUALS(obs->body, bodyId)) {
            return obs->damage_multiplier;
        }
    }
    return 1.0f;
}

ObstacleManager::~ObstacleManager() {
    obstacles.clear();
}