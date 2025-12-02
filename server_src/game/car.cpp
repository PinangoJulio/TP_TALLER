#include "car.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// ==========================================================
// CONSTRUCTOR
// ==========================================================

Car::Car(const std::string& model, const std::string& type, b2BodyId body)
    : model_name(model), 
      car_type(type),
      max_speed(100.0f), 
      acceleration(5000.0f), 
      handling(2.0f),
      max_durability(100.0f),
      nitro_boost(1.5f),      
      weight(1000.0f),       
      current_speed(0.0f),
      current_health(100.0f), 
      nitro_amount(100.0f), 
      nitro_active(false), 
      bodyId(body),
      drifting(false), 
      colliding(false),
      is_destroyed(false) {
    std::cout << "[Car] Creado: " << model << " (" << type << ")" << std::endl;
}

// ==========================================================
// CONFIGURAR STATS
// ==========================================================

void Car::load_stats(float max_spd, float accel, float hand, float durability, float nitro,
                     float wgt) {
    max_speed = max_spd;
    acceleration = accel * 50.0f;  // Escalar la aceleración para Box2D
    handling = hand;
    max_durability = durability;
    current_health = durability;  // Iniciar con salud completa
    nitro_boost = nitro;
    weight = wgt;

    std::cout << "[Car] Stats cargados para " << model_name << ":"
              << " Max Speed=" << max_speed << " Accel=" << acceleration 
              << " Handling=" << handling << " HP=" << max_durability << std::endl;
}

void Car::setBodyId(b2BodyId newBody) {
    this->bodyId = newBody;
}

// ==========================================================
// GETTERS FÍSICOS (DELEGADOS A BOX2D)
// ==========================================================

float Car::getX() const {
    if (B2_IS_NULL(bodyId)) return 0.0f; 
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    float meters_x = pos.x;
    float pixels_x = meters_x * 30.0f;  // Convertir metros a píxeles
    return pixels_x;
}

float Car::getY() const {
    if (B2_IS_NULL(bodyId)) return 0.0f; 
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    float meters_y = pos.y;
    float pixels_y = meters_y * 30.0f;  // Convertir metros a píxeles
    return pixels_y;
}

float Car::getAngle() const {
    if (B2_IS_NULL(bodyId)) return 0.0f; 
    b2Rot rot = b2Body_GetRotation(bodyId);
    return b2Rot_GetAngle(rot);
}

float Car::getVelocityX() const {
    if (B2_IS_NULL(bodyId)) return 0.0f;
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return vel.x;
}

float Car::getVelocityY() const {
    if (B2_IS_NULL(bodyId)) return 0.0f;
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return vel.y;
}

// ==========================================================
// SETTERS FÍSICOS (DELEGADOS A BOX2D)
// ==========================================================

void Car::setPosition(float nx, float ny) {
    if (B2_IS_NULL(bodyId)) return;

    float meters_x = nx / 30.0f;  // nx = pixel_x
    float meters_y = ny / 30.0f;  // ny = pixel_y
    
    b2Vec2 currentPos = {meters_x, meters_y};
    b2Rot currentRot = b2Body_GetRotation(bodyId);
    b2Body_SetTransform(bodyId, currentPos, currentRot);
}

void Car::setAngle(float angle_rad) {
    if (B2_IS_NULL(bodyId)) return;
    b2Vec2 currentPos = b2Body_GetPosition(bodyId);
    b2Rot newRot = b2MakeRot(angle_rad);
    b2Body_SetTransform(bodyId, currentPos, newRot);
}

void Car::setCurrentSpeed(float speed) {
    if (B2_IS_NULL(bodyId)) return;
    float angle = getAngle();
    b2Vec2 newVel = { std::cos(angle) * speed, std::sin(angle) * speed };
    b2Body_SetLinearVelocity(bodyId, newVel);
    current_speed = speed;
}

// ==========================================================
// SALUD Y DAÑO
// ==========================================================

void Car::takeDamage(float damage) {
    current_health = std::max(0.0f, current_health - damage);
    if (current_health <= 0) {
        is_destroyed = true;
        stop();
        std::cout << "[Car] " << model_name << " DESTRUIDO!" << std::endl;
    }
}

void Car::repair(float amount) {
    current_health = std::min(max_durability, current_health + amount);
    if (current_health > 0) {
        is_destroyed = false;
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
    if (nitro_active) {
        nitro_active = false;
        if (!B2_IS_NULL(bodyId)) {
            b2Body_SetLinearDamping(bodyId, 1.0f);
        }
    }
}

void Car::rechargeNitro(float amount) {
    nitro_amount = std::min(100.0f, nitro_amount + amount);
}

// ==========================================================
// COMANDOS DE CONTROL
// ==========================================================

void Car::accelerate(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return; 

    b2Body_SetLinearDamping(bodyId, 0.5f); 

    float currentSpeed = getCurrentSpeed();
    current_speed = currentSpeed;  // Actualizar velocidad actual

    if (currentSpeed < max_speed) {
        float angle = getAngle();
        b2Vec2 direction = { std::cos(angle), std::sin(angle) };

        float force = acceleration;
        if (nitro_active && nitro_amount > 0) {
            force *= nitro_boost;
            nitro_amount = std::max(0.0f, nitro_amount - (15.0f * delta_time));
            if (nitro_amount <= 0) {
                deactivateNitro();
            }
        }

        b2Vec2 forceVec = { direction.x * force, direction.y * force };
        b2Body_ApplyForceToCenter(bodyId, forceVec, true);
    } else {
        // Límite de velocidad
        b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
        float scale = max_speed / currentSpeed;
        b2Vec2 clampedVel = { velocity.x * scale, velocity.y * scale };
        b2Body_SetLinearVelocity(bodyId, clampedVel);
    }
}

void Car::brake(float delta_time) {
    (void)delta_time; // Delta time no se usa directamente
    if (is_destroyed || B2_IS_NULL(bodyId)) return;
    b2Body_SetLinearDamping(bodyId, 5.0f);
}

void Car::turn_left(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return;
    
    float turn_power = 0.3f * delta_time; 
    
    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang - turn_power);
}

void Car::turn_right(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return;

    float turn_power = 0.3f * delta_time;  // Reducido de 1.5f
    
    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang + turn_power);
}

void Car::stop() {
    if (B2_IS_NULL(bodyId)) return;
    b2Body_SetLinearVelocity(bodyId, {0.0f, 0.0f});
    b2Body_SetAngularVelocity(bodyId, 0.0f);
    current_speed = 0.0f;
}

// ==========================================================
// FÍSICA AVANZADA (DERRAJE)
// ==========================================================

void Car::updatePhysics() {
    if (B2_IS_NULL(bodyId)) return;


    alignWithVelocity();
    /*// 1. Obtener velocidad y ángulo actuales
    b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
    float angle = getAngle();
    current_speed = b2Length(velocity);  // Actualizar velocidad actual

    // 2. Calcular vectores de dirección
    // 0 grados = Derecha.
    b2Vec2 rightNormal = { std::sin(angle), -std::cos(angle) };

    // 3. Calcular velocidad lateral 
    float lateralSpeed = b2Dot(velocity, rightNormal);

    // 4. Aplicar impulso contrario para "matar" la velocidad lateral
    float grip = 0.90f;
    
    // Si estás derrapando (drifting = true), bajamos el grip
    if (drifting) grip = 0.5f;

    float impulseMagnitude = -lateralSpeed * weight * grip; 
    
    // Como 'weight' es alto (1000), reducimos la escala del impulso.
    impulseMagnitude *= 0.05f; 

    b2Vec2 impulse = { rightNormal.x * impulseMagnitude, rightNormal.y * impulseMagnitude };
    
    b2Body_ApplyLinearImpulseToCenter(bodyId, impulse, true);
    */
    // 5. Matar rotación residual (Angular Damping extra)
    b2Body_SetLinearDamping(bodyId, 0.2f);
    b2Body_SetAngularDamping(bodyId, 1.0f);
}

// ==========================================================
// RESET
// ==========================================================

void Car::reset() {
    stop();
    current_health = max_durability;
    nitro_amount = 100.0f;
    is_destroyed = false;
    nitro_active = false;
    drifting = false;
    colliding = false;
    current_speed = 0.0f;

    std::cout << "[Car] " << model_name << " reseteado" << std::endl;
}

void Car::alignWithVelocity() {
    if (B2_IS_NULL(bodyId)) return;
    
    b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
    float speed = b2Length(velocity);
    
    // Solo alinear si se está moviendo
    if (speed > 0.5f) {
        // Calcular ángulo de la velocidad
        float target_angle = std::atan2(velocity.y, velocity.x);
        
        // Obtener ángulo actual
        float current_angle = getAngle();
        
        // Calcular diferencia
        float angle_diff = target_angle - current_angle;
        
        // Normalizar diferencia (-π a π)
        while (angle_diff > M_PI) angle_diff -= 2.0f * M_PI;
        while (angle_diff < -M_PI) angle_diff += 2.0f * M_PI;
        
        // Aplicar torque para girar hacia la dirección del movimiento
        float torque = angle_diff * 100.0f;  // Fuerza de alineación
        
        b2Body_ApplyTorque(bodyId, torque, true);
    }
}