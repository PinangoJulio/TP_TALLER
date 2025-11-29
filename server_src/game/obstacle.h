#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <box2d/box2d.h>
#include <string>
#include <vector>

enum class ObstacleType {
    WALL,
    BARRIER,
    BRIDGE_PILLAR
};

struct Obstacle {
    ObstacleType type;
    b2BodyId body;
    float damage_multiplier;
    
    Obstacle(ObstacleType t, b2BodyId b, float dmg_mult = 1.0f)
        : type(t), body(b), damage_multiplier(dmg_mult) {}
};

class ObstacleManager {
private:
    std::vector<Obstacle> obstacles;
    b2WorldId world;

public:
    explicit ObstacleManager(b2WorldId world_id);
    
    void create_wall(float x, float y, float width, float height);
    void create_barrier(float x, float y, float length);
    void create_building(float x, float y, float width, float height);
    
    bool is_obstacle(b2BodyId body) const;
    float get_damage_multiplier(b2BodyId body) const;
    void clear();
    
    ~ObstacleManager();
};

#endif
