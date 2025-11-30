#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <vector>
#include <memory>
#include <box2d/box2d.h>

struct Obstacle {
    b2BodyId body;
    float damage_multiplier;
    bool is_static;
    
    Obstacle(b2BodyId b, float dmg, bool stat) 
        : body(b), damage_multiplier(dmg), is_static(stat) {}
};

class ObstacleManager {
private:
    std::vector<std::unique_ptr<Obstacle>> obstacles;

public:
    ObstacleManager() = default;

    // Métodos de creación que reciben el mundo explícitamente
    void create_wall(b2WorldId world, float x, float y, float width, float height);
    void create_building(b2WorldId world, float x, float y, float width, float height);
    void create_barrier(b2WorldId world, float x, float y, float length);

    // Consultas
    bool is_obstacle(b2BodyId bodyId) const;
    float get_damage_multiplier(b2BodyId bodyId) const;
    
    ~ObstacleManager();
};

#endif