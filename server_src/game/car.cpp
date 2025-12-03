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
      physics_body_data(nullptr),
      has_physics_body(false),   
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
// CONFIGURAR STATS (desde config.yaml)
// ==========================================================

void Car::load_stats(float max_spd, float accel, float hand, float durability, float nitro,
                     float wgt) {
    max_speed = max_spd;
    acceleration = accel;
    handling = hand;
    max_durability = durability;
    current_health = durability;  // Iniciar con salud completa
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
// C++
void Car::update(float delta_time) {
    (void)delta_time; //esto va solo si se usa box2d, porque no iria el otro codigo
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

void Car::accelerate(float delta_time) {
    if (is_destroyed)
        return;

    float effective_acceleration = acceleration;
    if (nitro_active) {
        effective_acceleration *= nitro_boost;
        // Consumir nitro
        nitro_amount = std::max(0.0f, nitro_amount - (20.0f * delta_time));
        if (nitro_amount <= 0) {
            deactivateNitro();
        }
    }

    //Con Box2d
    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);

    if (has_physics_body && physics_body_data) {
        b2BodyId bodyId = *static_cast<b2BodyId*>(physics_body_data);
        if (b2Body_IsValid(bodyId)) {
            // Convertir velocidad de píxeles/seg a metros/seg
            float vx_m = pixelsToMeters(velocity_x);
            float vy_m = pixelsToMeters(velocity_y);
            b2Vec2 vel = {vx_m, vy_m};
            
            b2Body_SetLinearVelocity(bodyId, vel);
            
            b2Body_SetAwake(bodyId, true);
        }
    }

    current_speed = std::min(max_speed, current_speed + effective_acceleration * delta_time);

    update(delta_time);
}

// Nuevos métodos para movimiento en 4 direcciones fijas
void Car::move_up(float delta_time) {
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

    // Acelerar hacia arriba (dirección Y negativa)
    velocity_y -= effective_acceleration * delta_time;

    // Limitar velocidad máxima
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }

    // Actualizar ángulo para que apunte en la dirección del movimiento
    if (total_speed > 1.0f) {
        angle = std::atan2(velocity_y, velocity_x);
    }

    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::move_down(float delta_time) {
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

    // Acelerar hacia abajo (dirección Y positiva)
    velocity_y += effective_acceleration * delta_time;

    // Limitar velocidad máxima
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }

    // Actualizar ángulo para que apunte en la dirección del movimiento
    if (total_speed > 1.0f) {
        angle = std::atan2(velocity_y, velocity_x);
    }

    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::move_left(float delta_time) {
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

    // Acelerar hacia la izquierda (dirección X negativa)
    velocity_x -= effective_acceleration * delta_time;

    // Limitar velocidad máxima
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }

    // Actualizar ángulo para que apunte en la dirección del movimiento
    if (total_speed > 1.0f) {
        angle = std::atan2(velocity_y, velocity_x);
    }

    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::move_right(float delta_time) {
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

    // Acelerar hacia la derecha (dirección X positiva)
    velocity_x += effective_acceleration * delta_time;

    // Limitar velocidad máxima
    float total_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
    if (total_speed > max_speed) {
        float scale = max_speed / total_speed;
        velocity_x *= scale;
        velocity_y *= scale;
    }

    // Actualizar ángulo para que apunte en la dirección del movimiento
    if (total_speed > 1.0f) {
        angle = std::atan2(velocity_y, velocity_x);
    }

    current_speed = total_speed;
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
}

void Car::brake(float delta_time) {
    if (is_destroyed)
        return;

    float brake_force = acceleration * 2.0f;  // Frenar es más rápido que acelerar
    current_speed = std::max(0.0f, current_speed - brake_force * delta_time);

    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);
}

void Car::apply_friction(float delta_time) {
    if (is_destroyed)
        return;

    // Aplicar fricción suave: 95% de la velocidad por segundo
    float friction_factor = std::pow(0.05f, delta_time);  // Más suave que brake
    current_speed *= friction_factor;

    // Parar completamente si la velocidad es muy baja
    if (current_speed < 0.5f) {
        current_speed = 0.0f;
        velocity_x = 0.0f;
        velocity_y = 0.0f;
    } else {
        // Actualizar componentes de velocidad según el ángulo actual
        velocity_x = current_speed * std::cos(angle);
        velocity_y = current_speed * std::sin(angle);

        // Actualizar posición con la nueva velocidad
        x += velocity_x * delta_time;
        y += velocity_y * delta_time;
    }
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

// ============================================
// MÉTODOS BOX2D V3
// ============================================

// Turn aplicando box2d
void Car::turn_left(float delta_time) {
    // 1. Validaciones básicas (igual que antes)
    if (is_destroyed || current_speed < 1.0f) // Bajé un poco el límite para que sea más responsivo
        return;

    // 2. Calcular nuevo ángulo
    float turn_rate = handling * delta_time;
    angle -= turn_rate;

    // Normalizar ángulo (Matemática básica)
    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;

    
        b2BodyId bodyId = *static_cast<b2BodyId*>(physics_body_data);
        
        if (b2Body_IsValid(bodyId)) {
            // A) Rotar el cuerpo físico en el mundo
            // Obtenemos la posición actual para no moverlo, solo rotarlo
            b2Vec2 position = b2Body_GetPosition(bodyId);
            b2Rot newRotation = b2MakeRot(angle);
            b2Body_SetTransform(bodyId, position, newRotation);

            // B) "Agarre de neumáticos": Redirigir la velocidad actual al nuevo ángulo
            // Si no haces esto, el auto gira pero sigue resbalando hacia la dirección vieja (Drift infinito)
            
            // Calculamos el vector de dirección basado en el NUEVO ángulo
            float vx_new = current_speed * std::cos(angle);
            float vy_new = current_speed * std::sin(angle);
            
            // Convertimos a Metros para Box2D
            b2Vec2 velocity_vector = {
                pixelsToMeters(vx_new), 
                pixelsToMeters(vy_new)
            };

            // Aplicamos la nueva velocidad redirigida
            b2Body_SetLinearVelocity(bodyId, velocity_vector);
            
            // Despertar al cuerpo
            b2Body_SetAwake(bodyId, true);
        }

}

void Car::turn_right(float delta_time) {
    if (is_destroyed || current_speed < 1.0f)
        return;

    float turn_rate = handling * delta_time;
    angle += turn_rate;

    // Normalizar ángulo
    while (angle < 0) angle += 2.0f * M_PI;
    while (angle >= 2.0f * M_PI) angle -= 2.0f * M_PI;

    // --- BOX2D LOGIC ---
    if (has_physics_body && physics_body_data) {
        b2BodyId bodyId = *static_cast<b2BodyId*>(physics_body_data);
        
        if (b2Body_IsValid(bodyId)) {
            // A) Rotar cuerpo
            b2Vec2 position = b2Body_GetPosition(bodyId);
            b2Rot newRotation = b2MakeRot(angle);
            b2Body_SetTransform(bodyId, position, newRotation);

            // B) Redirigir velocidad (Grip)
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
    
    // Si ya existía un body, destruirlo
    if (has_physics_body && physics_body_data) {
        b2BodyId* old_body_id = static_cast<b2BodyId*>(physics_body_data);
        if (b2Body_IsValid(*old_body_id)) {
            b2DestroyBody(*old_body_id);
        }
        delete old_body_id;
        physics_body_data = nullptr;
        has_physics_body = false;
    }
    
    // Convertir píxeles a metros
    float spawn_x_m = pixelsToMeters(spawn_x_px);
    float spawn_y_m = pixelsToMeters(spawn_y_px);
    
    std::cout << "[Car " << model_name << "] Creating physics body:" << std::endl;
    std::cout << "  Pixels: (" << spawn_x_px << ", " << spawn_y_px << ")" << std::endl;
    std::cout << "  Meters: (" << spawn_x_m << ", " << spawn_y_m << ")" << std::endl;
    
    // Definir el cuerpo (Box2D v3)
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {spawn_x_m, spawn_y_m};
    bodyDef.rotation = b2MakeRot(spawn_angle);
    bodyDef.linearDamping = 1.0f;
    bodyDef.angularDamping = 2.0f;
    
    b2BodyId body_id = b2CreateBody(world_id, &bodyDef);
    
    // Crear forma (cuadrado)
    float half_size_m = pixelsToMeters(car_size_px) / 2.0f;
    b2Polygon boxShape = b2MakeBox(half_size_m, half_size_m);
    
    std::cout << "  Box2D size: " << (half_size_m * 2.0f) << "m x " << (half_size_m * 2.0f) << "m" << std::endl;
    
    // Definir propiedades físicas (Box2D v3)
    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 10.0f;
    
    b2CreatePolygonShape(body_id, &shapeDef, &boxShape);
    
    // Guardar el body_id en memoria heap
    physics_body_data = new b2BodyId(body_id);
    has_physics_body = true;
    
    // Sincronizar variables internas
    x = spawn_x_px;
    y = spawn_y_px;
    angle = spawn_angle;
    
    std::cout << "  Body created successfully!" << std::endl;
}

void Car::syncFromPhysics() {
    if (!has_physics_body || !physics_body_data) return;
    
    b2BodyId body_id = *static_cast<b2BodyId*>(physics_body_data);
    
    if (B2_IS_NULL(body_id)) return;
    
    // Leer posición (Box2D v3)
    b2Vec2 pos_m = b2Body_GetPosition(body_id);
    x = metersToPixels(pos_m.x);
    y = metersToPixels(pos_m.y);
    
    // Leer rotación (Box2D v3)
    b2Rot rotation = b2Body_GetRotation(body_id);
    angle = b2Rot_GetAngle(rotation);
    
    // Leer velocidad
    b2Vec2 vel_m = b2Body_GetLinearVelocity(body_id);
    velocity_x = metersToPixels(vel_m.x);
    velocity_y = metersToPixels(vel_m.y);
    
    current_speed = std::sqrt(velocity_x * velocity_x + velocity_y * velocity_y);
}

void Car::destroyPhysicsBody(void* world_id_ptr) {
    if (!has_physics_body || !physics_body_data || !world_id_ptr) return;
    
    b2WorldId world_id = *static_cast<b2WorldId*>(world_id_ptr);
    b2BodyId body_id = *static_cast<b2BodyId*>(physics_body_data);
    
    if (B2_IS_NON_NULL(body_id) && b2World_IsValid(world_id)) {
        b2DestroyBody(body_id);
    }
    
    delete static_cast<b2BodyId*>(physics_body_data);
    physics_body_data = nullptr;
    has_physics_body = false;
}

Car::~Car() {
    // Limpiar memoria del body si existe
    if (physics_body_data) {
        delete static_cast<b2BodyId*>(physics_body_data);
        physics_body_data = nullptr;
    }
}

