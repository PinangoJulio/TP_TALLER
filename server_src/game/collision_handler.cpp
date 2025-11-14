#include "collision_handler.h"
#include <cmath>
#include <iostream>

CollisionHandler::CollisionHandler() : obstacle_manager(nullptr) {}

void CollisionHandler::set_obstacle_manager(ObstacleManager* manager) {
    obstacle_manager = manager;
    std::cout << "[CollisionHandler] ObstacleManager linked" << std::endl;
}

void CollisionHandler::register_car(int player_id, Car* car) {
    car_map[player_id] = car;
    std::cout << "[CollisionHandler] Car " << player_id << " registered" << std::endl;
}

void CollisionHandler::unregister_car(int player_id) {
    car_map.erase(player_id);
}

void CollisionHandler::process_contact_event(b2ContactEvents events) {
    for (int i = 0; i < events.beginCount; ++i) {
        b2ContactBeginTouchEvent* event = events.beginEvents + i;
        
        b2BodyId bodyA = b2Shape_GetBody(event->shapeIdA);
        b2BodyId bodyB = b2Shape_GetBody(event->shapeIdB);
        
        void* userDataA = b2Body_GetUserData(bodyA);
        void* userDataB = b2Body_GetUserData(bodyB);
        
        // Calcular velocidades relativas
        b2Vec2 velA = b2Body_GetLinearVelocity(bodyA);
        b2Vec2 velB = b2Body_GetLinearVelocity(bodyB);
        b2Vec2 relative_vel = {velA.x - velB.x, velA.y - velB.y};
        float impact_force = std::sqrt(relative_vel.x * relative_vel.x + 
                                      relative_vel.y * relative_vel.y);
        
        // CASO 1: Colisión entre dos autos
        if (userDataA && userDataB) {
            int player_id_a = *static_cast<int*>(userDataA);
            int player_id_b = *static_cast<int*>(userDataB);
            
            CollisionEvent collision;
            collision.car_id_a = player_id_a;
            collision.car_id_b = player_id_b;
            collision.impact_force = impact_force;
            collision.is_with_obstacle = false;
            collision.damage_multiplier = 1.0f;
            
            pending_collisions.push_back(collision);
            
            std::cout << "[CollisionHandler] Car-to-Car collision! " 
                      << player_id_a << " <-> " << player_id_b 
                      << " | Force: " << impact_force << std::endl;
        }
        // CASO 2: Colisión auto-obstáculo (A es auto, B es obstáculo)
        else if (userDataA && !userDataB && obstacle_manager) {
            if (obstacle_manager->is_obstacle(bodyB)) {
                int player_id = *static_cast<int*>(userDataA);
                float damage_mult = obstacle_manager->get_damage_multiplier(bodyB);
                
                CollisionEvent collision;
                collision.car_id_a = player_id;
                collision.car_id_b = -1;  // -1 indica obstáculo
                collision.impact_force = impact_force;
                collision.is_with_obstacle = true;
                collision.damage_multiplier = damage_mult;
                
                pending_collisions.push_back(collision);
                
                std::cout << "[CollisionHandler] Car-to-Obstacle collision! Car " 
                          << player_id << " | Force: " << impact_force 
                          << " | Multiplier: " << damage_mult << "x" << std::endl;
            }
        }
        // CASO 3: Colisión auto-obstáculo (B es auto, A es obstáculo)
        else if (!userDataA && userDataB && obstacle_manager) {
            if (obstacle_manager->is_obstacle(bodyA)) {
                int player_id = *static_cast<int*>(userDataB);
                float damage_mult = obstacle_manager->get_damage_multiplier(bodyA);
                
                CollisionEvent collision;
                collision.car_id_a = player_id;
                collision.car_id_b = -1;
                collision.impact_force = impact_force;
                collision.is_with_obstacle = true;
                collision.damage_multiplier = damage_mult;
                
                pending_collisions.push_back(collision);
                
                std::cout << "[CollisionHandler] Car-to-Obstacle collision! Car " 
                          << player_id << " | Force: " << impact_force 
                          << " | Multiplier: " << damage_mult << "x" << std::endl;
            }
        }
    }
}

void CollisionHandler::apply_pending_collisions() {
    for (const auto& collision : pending_collisions) {
        auto it_a = car_map.find(collision.car_id_a);
        
        if (collision.is_with_obstacle) {
            // COLISIÓN CON OBSTÁCULO
            if (it_a != car_map.end() && it_a->second->is_alive()) {
                float adjusted_force = collision.impact_force * collision.damage_multiplier;
                it_a->second->apply_collision_damage(adjusted_force);
                
                std::cout << "[CollisionHandler] Applied obstacle damage to Car " 
                          << collision.car_id_a 
                          << " | Force: " << adjusted_force << std::endl;
            }
        } else {
            // COLISIÓN ENTRE AUTOS
            auto it_b = car_map.find(collision.car_id_b);
            
            if (it_a != car_map.end() && it_a->second->is_alive()) {
                it_a->second->apply_collision_damage(collision.impact_force);
            }
            
            if (it_b != car_map.end() && it_b->second->is_alive()) {
                it_b->second->apply_collision_damage(collision.impact_force);
            }
        }
    }
}

void CollisionHandler::clear_collisions() {
    pending_collisions.clear();
}
