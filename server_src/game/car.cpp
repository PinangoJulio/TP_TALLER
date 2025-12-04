#include "car.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include <box2d/box2d.h>
#include "physics_constants.h"

// ==========================================================
// CONSTRUCTOR
// ==========================================================

Car::Car(const std::string& model, const std::string& type)
    : model_name(model), car_type(type), max_speed(100.0f), acceleration(50.0f), handling(1.0f),
      max_durability(100.0f), nitro_boost(1.5f), weight(1000.0f), current_speed(0.0f),
      current_health(100.0f), nitro_amount(100.0f), nitro_active(false), x(0.0f), y(0.0f),
      angle(0.0f), velocity_x(0.0f), velocity_y(0.0f), 
      // Inicializamos el ID como nulo según la norma Box2D v3
      bodyId(b2_nullBodyId),
      car_size_px(40.0f),        
      is_drifting(false),        
      is_colliding(false),       
      is_destroyed(false)        
{
    if (type == "classic" || type == "small") {
        car_size_px = CAR_SMALL_SIZE;
    } else if (type == "sport" || type == "medium") {
        car_size_px = CAR_MEDIUM_SIZE;
    } else {
        car_size_px = CAR_LARGE_SIZE;
    }

    std::cout << "[Car] Creado: " << model << " (" << type << ")" << std::endl;
}

// ==========================================================
// CONFIGURAR STATS
// ==========================================================

void Car::load_stats(float max_spd, float accel, float hand, float durability, float nitro,
                     float wgt) {
    max_speed = max_spd;
    acceleration = accel;
    handling = hand;
    max_durability = durability;
    current_health = durability;
    nitro_boost = nitro;
    weight = wgt;

    std::cout << "[Car] Stats cargados para " << model_name << ":"
              << " Max Speed=" << max_speed << " Accel=" << acceleration << " Handling=" << handling
              << " HP=" << max_durability << std::endl;
}

// ==========================================================
// SALUD Y DAÑO
// ==========================================================

void Car::takeDamage(float damage) {
    current_health = std::max(0.0f, current_health - damage);
    if (current_health <= 0) {
        is_destroyed = true;
        current_speed = 0;
        std::cout << "[Car] " << model_name << " DESTRUIDO!" << std::endl;
    }
}

void Car::repair(float amount) {
    if (!is_destroyed) {
        current_health = std::min(max_durability, current_health + amount);
    }
}

// ==========================================================
// NITRO
// ==========================================================

void Car::activateNitro() {
    if (nitro_amount > 0 && !nitro_active && !is_destroyed) {
        nitro_active = true;
        std::cout << "[Car] " << model_name << " NITRO ACTIVADO!" << std::endl;
    }
}

void Car::deactivateNitro() {
    nitro_active = false;
}

void Car::rechargeNitro(float amount) {
    nitro_amount = std::min(100.0f, nitro_amount + amount);
}

// ==========================================================
// COMANDOS DE CONTROL
// ==========================================================

void Car::update(float delta_time) {
    (void)delta_time;
    if (is_destroyed)
        return;

    /*
    // --- LÓGICA LEGACY (Solo se ejecuta si NO hay Box2D) ---
    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);

    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
    */
}

/*Sin box2d
void Car::accelerate(float delta_time) {
    if (is_destroyed)
        return;

    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) {
            deactivateNitro();
        }
    }

    //Con Box2d
    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);

    // Usamos el bodyId directamente
    if (B2_IS_NON_NULL(bodyId)) {
        if (b2Body_IsValid(bodyId)) {
            float vx_m = pixelsToMeters(velocity_x);
            float vy_m = pixelsToMeters(velocity_y);
            b2Vec2 vel = {vx_m, vy_m};
            
            b2Body_SetLinearVelocity(bodyId, vel);
            b2Body_SetAwake(bodyId, true);
        }
    }

    current_speed = std::min(max_speed, current_speed + effective_acceleration * delta_time);
    update(delta_time);
}*/


//con box2d
void Car::accelerate(float delta_time) {
    if (is_destroyed) return;
    
    if (B2_IS_NULL(bodyId) || !b2Body_IsValid(bodyId)) return;

    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }

    current_speed = std::min(max_speed, current_speed + effective_acceleration * delta_time);
    
    float fx = current_speed * std::cos(angle);
    float fy = current_speed * std::sin(angle);
    
    b2Vec2 velocity = {pixelsToMeters(fx), pixelsToMeters(fy)};
    b2Body_SetLinearVelocity(bodyId, velocity);
    b2Body_SetAwake(bodyId, true);
}

// ... MOVIMIENTO EN 4 DIRECCIONES...

/* sin box2d
void Car::move_up(float delta_time) {
    if (is_destroyed) return;
    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }
    velocity_y -= effective_acceleration * delta_time;
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }
    if (total_speed > 1.0f) angle = std::atan2(velocity_y, velocity_x);
    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}*/

//Con box2d

void Car::move_up(float delta_time) {
    if (is_destroyed) return;
    if (B2_IS_NULL(bodyId) || !b2Body_IsValid(bodyId)) return;

    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }

    b2Vec2 currentVel = b2Body_GetLinearVelocity(bodyId);
    currentVel.y -= pixelsToMeters(effective_acceleration * delta_time);
    
    // Limitar velocidad máxima
    float speed = std::sqrt(currentVel.x * currentVel.x + currentVel.y * currentVel.y);
    if (speed > pixelsToMeters(max_speed)) {
        float scale = pixelsToMeters(max_speed) / speed;
        currentVel.x *= scale;
        currentVel.y *= scale;
    }
    
    b2Body_SetLinearVelocity(bodyId, currentVel);
    b2Body_SetAwake(bodyId, true);
    
    syncFromPhysics();
}

// Replicar lo mismo para move_down, move_left, move_right

void Car::move_down(float delta_time) {
    if (is_destroyed) return;
    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }
    velocity_y += effective_acceleration * delta_time;
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }
    if (total_speed > 1.0f) angle = std::atan2(velocity_y, velocity_x);
    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::move_left(float delta_time) {
    if (is_destroyed) return;
    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }
    velocity_x -= effective_acceleration * delta_time;
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }
    if (total_speed > 1.0f) angle = std::atan2(velocity_y, velocity_x);
    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::move_right(float delta_time) {
    if (is_destroyed) return;
    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) deactivateNitro();
    }
    velocity_x += effective_acceleration * delta_time;
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }
    if (total_speed > 1.0f) angle = std::atan2(velocity_y, velocity_x);
    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

//sin box2d
/*
void Car::brake(float delta_time) {
    if (is_destroyed)
        return;

    float brake_force = acceleration * 2.0f;
    current_speed = std::max(0.0f, current_speed - brake_force * delta_time);

    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);
}*/

//con box2d
void Car::brake(float delta_time) {
    if (is_destroyed) return;
    if (B2_IS_NULL(bodyId) || !b2Body_IsValid(bodyId)) return;

    float brake_force = acceleration * 2.0f;
    current_speed = std::max(0.0f, current_speed - brake_force * delta_time);

    b2Vec2 currentVel = b2Body_GetLinearVelocity(bodyId);
    float scale = (current_speed / max_speed) * 0.5f; // Frenado gradual
    b2Body_SetLinearVelocity(bodyId, {currentVel.x * scale, currentVel.y * scale});
}
//Sin box2d
/*
void Car::apply_friction(float delta_time) {
    if (is_destroyed)
        return;

    float friction_factor = std::pow(0.05f, delta_time);
    current_speed *= friction_factor;

    if (current_speed < 0.5f) {
        current_speed = 0.0f;
        velocity_x = 0.0f;
        velocity_y = 0.0f;
    } else {
        velocity_x = current_speed * std::cos(angle);
        velocity_y = current_speed * std::sin(angle);

        x += velocity_x * delta_time;
        y += velocity_y * delta_time;
    }
}*/

//con box2d
void Car::apply_friction(float delta_time) {
    (void)delta_time;  

    if (is_destroyed) return;
    if (B2_IS_NULL(bodyId) || !b2Body_IsValid(bodyId)) return;

    syncFromPhysics();
    
    if (current_speed < 0.5f) {
        current_speed = 0.0f;
        b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
    }
}

// resetear para una nueva carrera
void Car::reset() {
    current_speed = 0.0f;
    current_health = max_durability;
    nitro_amount = 100.0f;
    nitro_active = false;
    x = 0.0f;
    y = 0.0f;
    angle = 0.0f;
    velocity_x = 0.0f;
    velocity_y = 0.0f;
    is_drifting = false;
    is_colliding = false;
    is_destroyed = false;

    std::cout << "[Car] " << model_name << " reseteado" << std::endl;
}


//Turn left y right sin box2d
/*
void Car::turn_left(float delta_time) {
    if (is_destroyed || current_speed < 5.0f)
        return;  // No girar si está muy lento

    float turn_rate = handling * delta_time;
    angle -= turn_rate;

    // Normalizar ángulo
    while (angle < 0)
        angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI)
        angle -= 2.0f * M_PI;
}

void Car::turn_right(float delta_time) {
    if (is_destroyed || current_speed < 5.0f)
        return;

    float turn_rate = handling * delta_time;
    angle += turn_rate;

    // Normalizar ángulo
    while (angle < 0)
        angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI)
        angle -= 2.0f * M_PI;
}*/

// ============================================
// MÉTODOS BOX2D V3
// ============================================

void Car::turn_left(float delta_time) {
    if (is_destroyed || current_speed < 1.0f)
        return;

    float turn_rate = handling * delta_time;
    angle -= turn_rate;

    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;

    // ARREGLADO: Verificación de seguridad y uso de variable miembro
    if (B2_IS_NON_NULL(bodyId) && b2Body_IsValid(bodyId)) {
        b2Vec2 position = b2Body_GetPosition(bodyId);
        b2Rot newRotation = b2MakeRot(angle);
        b2Body_SetTransform(bodyId, position, newRotation);

        float vx_new = current_speed * std::cos(angle);
        float vy_new = current_speed * std::sin(angle);
        
        b2Vec2 velocity_vector = {
            pixelsToMeters(vx_new), 
            pixelsToMeters(vy_new)
        };

        b2Body_SetLinearVelocity(bodyId, velocity_vector);
        b2Body_SetAwake(bodyId, true);
    }
}

void Car::turn_right(float delta_time) {
    if (is_destroyed || current_speed < 1.0f)
        return;

    float turn_rate = handling * delta_time;
    angle += turn_rate;

    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;

    // ARREGLADO: Uso de variable miembro directa
    if (B2_IS_NON_NULL(bodyId) && b2Body_IsValid(bodyId)) {
        b2Vec2 position = b2Body_GetPosition(bodyId);
        b2Rot newRotation = b2MakeRot(angle);
        b2Body_SetTransform(bodyId, position, newRotation);

        float vx_new = current_speed * std::cos(angle);
        float vy_new = current_speed * std::sin(angle);
        
        b2Vec2 velocity_vector = {
            pixelsToMeters(vx_new), 
            pixelsToMeters(vy_new)
        };

        b2Body_SetLinearVelocity(bodyId, velocity_vector);
        b2Body_SetAwake(bodyId, true);
    }
}

void Car::createPhysicsBody(void* world_id_ptr, float spawn_x_px, float spawn_y_px, float spawn_angle) {
    if (!world_id_ptr) {
        std::cerr << "[Car] ERROR: Null world ID!" << std::endl;
        return;
    }
    
    b2WorldId world_id = *static_cast<b2WorldId*>(world_id_ptr);
    
    if (!b2World_IsValid(world_id)) {
        std::cerr << "[Car] ERROR: Invalid world ID!" << std::endl;
        return;
    }
    
    // Si ya existía un body, destruirlo (ARREGLADO para usar bodyId)
    if (B2_IS_NON_NULL(bodyId)) {
        if (b2Body_IsValid(bodyId)) {
            b2DestroyBody(bodyId);
        }
        bodyId = b2_nullBodyId;
    }
    
    float spawn_x_m = pixelsToMeters(spawn_x_px);
    float spawn_y_m = pixelsToMeters(spawn_y_px);
    
    std::cout << "[Car " << model_name << "] Creating physics body at Meters: (" 
              << spawn_x_m << ", " << spawn_y_m << ")" << std::endl;
    
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {spawn_x_m, spawn_y_m};
    bodyDef.rotation = b2MakeRot(spawn_angle);
    bodyDef.linearDamping = 1.0f;
    bodyDef.angularDamping = 2.0f;
    
    // ARREGLADO: Asignación directa a la variable de clase (¡Esto arregla el Getter!)
    this->bodyId = b2CreateBody(world_id, &bodyDef);
    
    float half_size_m = pixelsToMeters(car_size_px) / 2.0f;
    b2Polygon boxShape = b2MakeBox(half_size_m, half_size_m);
    
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 10.0f;
    
    b2CreatePolygonShape(this->bodyId, &shapeDef, &boxShape);
    
    x = spawn_x_px;
    y = spawn_y_px;
    angle = spawn_angle;
    
    std::cout << "  Body created successfully!" << std::endl;
}

void Car::syncFromPhysics() {
    // ARREGLADO: Verificación usando ID directo
    if (B2_IS_NULL(bodyId)) return;
    
    b2Vec2 pos_m = b2Body_GetPosition(bodyId);
    x = metersToPixels(pos_m.x);
    y = metersToPixels(pos_m.y);
    
    b2Rot rotation = b2Body_GetRotation(bodyId);
    angle = b2Rot_GetAngle(rotation);
    
    b2Vec2 vel_m = b2Body_GetLinearVelocity(bodyId);
    velocity_x = metersToPixels(vel_m.x);
    velocity_y = metersToPixels(vel_m.y);
    
    current_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
}

void Car::destroyPhysicsBody(void* world_id_ptr) {
    // 1. Silenciamos el warning de variable no usada
    (void)world_id_ptr; 

    // 2. Lógica de destrucción
    if (B2_IS_NULL(bodyId)) return;
    
    // Solo destruimos si el ID es válido
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }
    
    // Reseteamos el ID a nulo
    bodyId = b2_nullBodyId;
}

Car::~Car() {
}


//Friccion lateral
/*
void Car::updatePhysics() {
    if (!has_physics_body || !physics_body_data) return;
    
    b2BodyId bodyId = *static_cast<b2BodyId*>(physics_body_data);
    if (!b2Body_IsValid(bodyId)) return;
    
    // Aplicar fricción lateral (drift control)
    b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
    b2Rot rotation = b2Body_GetRotation(bodyId);
    
    // Vector forward del auto
    float forward_x = rotation.c;
    float forward_y = rotation.s;
    
    // Componente lateral de la velocidad
    float lateral_dot = velocity.x * (-forward_y) + velocity.y * forward_x;
    
    // Fricción lateral (agarre)
    float lateral_friction = 0.95f; // 95% de agarre
    b2Vec2 lateral_impulse = {
        -lateral_dot * (-forward_y) * lateral_friction,
        -lateral_dot * forward_x * lateral_friction
    };
    
    b2Body_ApplyLinearImpulseToCenter(bodyId, lateral_impulse, true);
}
*/
