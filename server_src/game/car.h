#ifndef SERVER_CAR_H
#define SERVER_CAR_H

#include <cstddef>
#include <box2d/box2d.h>

struct Car {
    int client_id;
    bool nitro_active;
    int nitro_ticks;
    int nitro_duration;
    
    int health;          
    float speed;         
    b2BodyId body;       
    bool is_destroyed;

    Car(int id, int max_duration, int initial_health = 100);

    int get_client_id() const { return client_id; }
    bool activate_nitro();
    bool simulate_tick();
    bool is_nitro_active() const { return nitro_active; }
    
    void apply_collision_damage(float impact_force);
    bool is_alive() const { return health > 0 && !is_destroyed; }
    void destroy();
};

#endif