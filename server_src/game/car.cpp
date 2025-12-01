#include "car.h"
#include <cmath>
#include <algorithm>
#include <iostream>

// ================= CONSTRUCTOR =================
Car::Car(const std::string& model, const std::string& type, b2BodyId body)
    : model_name(model), 
      car_type(type),
      max_speed(100.0f), 
      acceleration_power(5000.0f), 
      handling(2.0f),
      max_durability(100.0f),
      nitro_boost(1.5f),      
      weight(1000.0f),       
      current_health(100.0f), 
      nitro_amount(100.0f), 
      nitro_active(false), 
      is_destroyed(false), 
      drifting(false), 
      colliding(false),
      bodyId(body) {
}

// ================= CONFIGURACIÓN =================
void Car::load_stats(float max_spd, float accel, float hand, float durability, float nitro, float wgt) {
    this->max_speed = max_spd;
    this->acceleration_power = accel * 50.0f; 
    this->handling = hand;
    this->max_durability = durability;
    this->current_health = durability;
    this->nitro_boost = nitro;
    this->weight = wgt;
}

void Car::setBodyId(b2BodyId newBody) {
    this->bodyId = newBody;
}

// ================= FÍSICA (GETTERS PROTEGIDOS) =================
// Estos 'if' son los que evitan que el server se cierre en el Lobby

float Car::getX() const {
    if (B2_IS_NULL(bodyId)) return 0.0f; 
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return pos.x;
}

float Car::getY() const {
    if (B2_IS_NULL(bodyId)) return 0.0f; 
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return pos.y;
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

float Car::getCurrentSpeed() const {
    if (B2_IS_NULL(bodyId)) return 0.0f;
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return b2Length(vel);
}

// ================= FÍSICA (SETTERS PROTEGIDOS) =================

void Car::setPosition(float nx, float ny) {
    if (B2_IS_NULL(bodyId)) return;
    b2Vec2 currentPos = {nx, ny};
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
}

// ================= COMANDOS DE CONTROL =================

void Car::accelerate(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return; 

    float currentSpeed = getCurrentSpeed();

    if (currentSpeed < max_speed) {
        float angle = getAngle();
        b2Vec2 direction = { std::cos(angle), std::sin(angle) };

        float force = acceleration_power;
        if (nitro_active && nitro_amount > 0) {
            force *= nitro_boost;
            nitro_amount -= (15.0f * delta_time);
            if(nitro_amount <= 0) deactivateNitro();
        }

        b2Vec2 forceVec = { direction.x * force, direction.y * force };
        b2Body_ApplyForceToCenter(bodyId, forceVec, true);
    } else {
        // Límite de velocidad suave
        b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
        float scale = max_speed / currentSpeed;
        b2Vec2 clampedVel = { velocity.x * scale, velocity.y * scale };
        b2Body_SetLinearVelocity(bodyId, clampedVel);
    }
}

void Car::brake(float delta_time) {
    (void)delta_time; 
    if (is_destroyed || B2_IS_NULL(bodyId)) return;
    b2Body_SetLinearDamping(bodyId, 5.0f); 
}

void Car::turn_left(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return;
    if (getCurrentSpeed() < 1.0f) return;

    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang - (handling * delta_time));
}

void Car::turn_right(float delta_time) {
    if (is_destroyed || B2_IS_NULL(bodyId)) return;
    if (getCurrentSpeed() < 1.0f) return;

    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang + (handling * delta_time));
}

void Car::stop() {
    if (B2_IS_NULL(bodyId)) return;
    b2Body_SetLinearVelocity(bodyId, {0,0});
    b2Body_SetAngularVelocity(bodyId, 0);
}

// ================= LÓGICA DE JUEGO =================

void Car::takeDamage(float damage) {
    current_health -= damage;
    if (current_health <= 0) {
        current_health = 0;
        is_destroyed = true;
    }
}

void Car::repair(float amount) {
    current_health = std::min(max_durability, current_health + amount);
    is_destroyed = false;
}

void Car::activateNitro() { 
    if (nitro_amount > 0) nitro_active = true; 
}

void Car::deactivateNitro() { 
    nitro_active = false; 
    if (!B2_IS_NULL(bodyId)) {
        b2Body_SetLinearDamping(bodyId, 1.0f);
    }
}

void Car::rechargeNitro(float amount) {
    nitro_amount = std::min(100.0f, nitro_amount + amount);
}

void Car::reset() {
    stop();
    current_health = max_durability;
    nitro_amount = 100.0f;
    is_destroyed = false;
    nitro_active = false;
    drifting = false;
    colliding = false;
}