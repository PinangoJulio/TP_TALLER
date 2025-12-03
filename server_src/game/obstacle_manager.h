#ifndef OBSTACLE_MANAGER_H
#define OBSTACLE_MANAGER_H

#include <box2d/box2d.h>
#include <vector>

class ObstacleManager {
private:
    std::vector<b2BodyId> obstacles;

public:
    void create_wall(b2WorldId world, float x, float y, float w, float h) {
        b2BodyDef bodyDef = b2DefaultBodyDef();
        bodyDef.type = b2_staticBody;
        bodyDef.position = {x, y};
        
        b2BodyId bodyId = b2CreateBody(world, &bodyDef);
        
        b2Polygon box = b2MakeBox(w / 2.0f, h / 2.0f);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        b2CreatePolygonShape(bodyId, &shapeDef, &box);
        
        obstacles.push_back(bodyId);
    }
    
    void clear() {
        obstacles.clear();
    }
};

#endif
