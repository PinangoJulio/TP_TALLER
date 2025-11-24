#include "car.h"
#include <cmath>
#include <iostream>

Car::Car(int id, int max_duration, int initial_health)
    : client_id(id), 
      nitro_active(false), 
      nitro_ticks(0), 
      nitro_duration(max_duration),
      health(initial_health),
      speed(0.0f),
      is_destroyed(false) {
    body = b2_nullBodyId;
}

bool Car::activate_nitro() {
    if (nitro_active || !is_alive())
        return false;
    nitro_active = true;
    nitro_ticks = 0;
    return true;
}

bool Car::simulate_tick() {
    if (!nitro_active)
        return false;
    nitro_ticks++;
    if (nitro_ticks >= nitro_duration) {
        nitro_active = false;
        nitro_ticks = 0;
        return true;
    }
    return false;
}

void Car::apply_collision_damage(float impact_force) {
    if (is_destroyed) return;
    
    int damage = static_cast<int>(impact_force * 0.5f);
    
    if (impact_force > 50.0f) {
        std::cout << "[Car " << client_id << "] SEVERE collision! Force: " 
                  << impact_force << ", Damage: " << damage << std::endl;
    } else if (impact_force > 20.0f) {
        std::cout << "[Car " << client_id << "] Medium collision! Force: " 
                  << impact_force << ", Damage: " << damage << std::endl;
    } else {
        std::cout << "[Car " << client_id << "] Light collision. Force: " 
                  << impact_force << ", Damage: " << damage << std::endl;
    }
    
    health -= damage;
    if (health < 0) health = 0;
    
    float speed_reduction = damage * 0.02f;
    if (B2_IS_NON_NULL(body)) {
        b2Vec2 vel = b2Body_GetLinearVelocity(body);
        float current_speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        float new_speed = current_speed * (1.0f - speed_reduction);
        
        float health_factor = static_cast<float>(health) / 100.0f;
        if (health_factor < 0.5f) {
            new_speed *= (0.5f + health_factor);
        }
        
        if (current_speed > 0.01f) {
            float factor = new_speed / current_speed;
            b2Body_SetLinearVelocity(body, {vel.x * factor, vel.y * factor});
        }
    }
    
    if (health <= 0 && !is_destroyed) { 
        destroy();
    }
}

void Car::destroy() {
    if (is_destroyed) return;
    
    is_destroyed = true;
    health = 0;
    nitro_active = false;
    
    if (B2_IS_NON_NULL(body)) {
        b2Body_SetLinearVelocity(body, {0.0f, 0.0f});
        b2Body_SetAngularVelocity(body, 0.0f);
    }
    
    std::cout << "[Car " << client_id << "] DESTROYED!" << std::endl;
}
