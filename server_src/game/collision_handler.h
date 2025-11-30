#ifndef COLLISION_HANDLER_H
#define COLLISION_HANDLER_H

#include <box2d/box2d.h>
#include <map>
#include <vector>
#include "car.h"
#include "obstacle.h"

struct PhysicsCollisionEvent {
    int car_id_a;
    int car_id_b;
    float impact_force;
    bool is_with_obstacle;
    float damage_multiplier; 
};

class CollisionHandler {
private:
    std::map<int, Car*> car_map;
    std::vector<PhysicsCollisionEvent> pending_collisions;
    ObstacleManager* obstacle_manager;

public:
    CollisionHandler();
    
    void set_obstacle_manager(ObstacleManager* manager);
    void register_car(int player_id, Car* car);
    void unregister_car(int player_id);
    
    void process_contact_event(b2ContactEvents events);
    void apply_pending_collisions();
    void clear_collisions();
};

#endif