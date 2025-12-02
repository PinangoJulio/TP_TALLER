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
        
        // Datos físicos
        b2Vec2 velA = b2Body_GetLinearVelocity(bodyA);
        b2Vec2 velB = b2Body_GetLinearVelocity(bodyB);
        b2Vec2 posA = b2Body_GetPosition(bodyA);
        b2Vec2 posB = b2Body_GetPosition(bodyB);

        // Vector de velocidad relativa
        b2Vec2 relVel = {velA.x - velB.x, velA.y - velB.y};
        
        // CORRECCIÓN: Eliminada la variable 'impactSpeed' que no se usaba
        // (Ya usamos impactFactor abajo que es más preciso para el TP)

        // Calcular vector normal del choque (dirección de A hacia B)
        b2Vec2 normal = {posB.x - posA.x, posB.y - posA.y};
        float dist = b2Length(normal);
        if (dist > 0.0001f) {
            normal = {normal.x / dist, normal.y / dist}; // Normalizar
        } else {
            normal = {1, 0};
        }

        // --- CÁLCULO DEL ÁNGULO DE IMPACTO (Requisito TP) ---
        // Proyectamos la velocidad relativa sobre la normal del choque.
        // abs(dot) -> 1.0 es choque perpendicular (frontal/T-bone), 0.0 es roce.
        float impactFactor = std::abs(b2Dot(relVel, normal));
        
        // Si el impacto es muy lento (menos de 2 m/s aprox), ignoramos daño
        if (impactFactor < 2.0f) impactFactor = 0.0f; 

        if (userDataA && userDataB) {
            // Choque Auto vs Auto
            int player_id_a = *static_cast<int*>(userDataA);
            int player_id_b = *static_cast<int*>(userDataB);
            
            PhysicsCollisionEvent collision;
            collision.car_id_a = player_id_a;
            collision.car_id_b = player_id_b;
            
            // Usamos el impactFactor para modular la fuerza real
            // Frontal: Daño total. Lateral: Daño reducido.
            collision.impact_force = impactFactor; 
            
            collision.is_with_obstacle = false;
            collision.damage_multiplier = 1.0f; // Base
            
            pending_collisions.push_back(collision);
        }
        else if (userDataA && !userDataB && obstacle_manager) {
            // Auto A vs Obstáculo
            if (obstacle_manager->is_obstacle(bodyB)) {
                int player_id = *static_cast<int*>(userDataA);
                float damage_mult = obstacle_manager->get_damage_multiplier(bodyB);
                
                PhysicsCollisionEvent collision;
                collision.car_id_a = player_id;
                collision.car_id_b = -1;
                
                // En obstáculos, también importa el ángulo (chocar de frente vs raspar la pared)
                collision.impact_force = impactFactor;
                
                collision.is_with_obstacle = true;
                collision.damage_multiplier = damage_mult;
                pending_collisions.push_back(collision);
            }
        }
        else if (!userDataA && userDataB && obstacle_manager) {
            // Obstáculo vs Auto B
            if (obstacle_manager->is_obstacle(bodyA)) {
                int player_id = *static_cast<int*>(userDataB);
                float damage_mult = obstacle_manager->get_damage_multiplier(bodyA);
                
                PhysicsCollisionEvent collision;
                collision.car_id_a = player_id;
                collision.car_id_b = -1;
                collision.impact_force = impactFactor;
                collision.is_with_obstacle = true;
                collision.damage_multiplier = damage_mult;
                pending_collisions.push_back(collision);
            }
        }
    }
}

void CollisionHandler::apply_pending_collisions() {
    for (const auto& collision : pending_collisions) {
        auto it_a = car_map.find(collision.car_id_a);
        
        if (collision.is_with_obstacle) {
            if (it_a != car_map.end() && it_a->second->is_alive()) {
                float adjusted_force = collision.impact_force * collision.damage_multiplier;
                it_a->second->apply_collision_damage(adjusted_force);
                
                b2BodyId bodyId = it_a->second->getBodyId();
                if (B2_IS_NON_NULL(bodyId)) {
                    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
                    b2Vec2 bounce = {-vel.x * 0.5f, -vel.y * 0.5f};
                    b2Body_ApplyLinearImpulseToCenter(bodyId, bounce, true);
                }
            }
        } else {
            auto it_b = car_map.find(collision.car_id_b);
            
            if (it_a != car_map.end() && it_a->second->is_alive()) {
                it_a->second->apply_collision_damage(collision.impact_force);
            }
            
            if (it_b != car_map.end() && it_b->second->is_alive()) {
                it_b->second->apply_collision_damage(collision.impact_force);
            }
            
            // Rebote simple entre autos (opcional, Box2D ya maneja colisiones físicas)
            // Aquí solo aplicamos una corrección de velocidad si queremos un efecto arcade exagerado
            if (it_a != car_map.end() && it_b != car_map.end()) {
                b2BodyId bodyA = it_a->second->getBodyId();
                b2BodyId bodyB = it_b->second->getBodyId();

                if (B2_IS_NON_NULL(bodyA) && B2_IS_NON_NULL(bodyB)) {
                    b2Vec2 vel_a = b2Body_GetLinearVelocity(bodyA);
                    b2Vec2 vel_b = b2Body_GetLinearVelocity(bodyB);
                    
                    // Intercambio parcial de momentum (efecto choque elástico)
                    b2Body_SetLinearVelocity(bodyA, 
                        {vel_a.x * 0.3f + vel_b.x * 0.7f, 
                         vel_a.y * 0.3f + vel_b.y * 0.7f});
                    
                    b2Body_SetLinearVelocity(bodyB, 
                        {vel_b.x * 0.3f + vel_a.x * 0.7f, 
                         vel_b.y * 0.3f + vel_a.y * 0.7f});
                }
            }
        }
    }
}

void CollisionHandler::clear_collisions() {
    pending_collisions.clear();
}