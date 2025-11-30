#include "car.h"
#include <cmath>
#include <algorithm>
#include <iostream>

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

void Car::load_stats(float max_spd, float accel, float hand, float durability, float nitro, float wgt) {
    this->max_speed = max_spd;
    this->acceleration_power = accel * 50.0f; 
    this->handling = hand;
    this->max_durability = durability;
    this->current_health = durability;
    this->nitro_boost = nitro;
    this->weight = wgt;
}

// ================= FÍSICA (GETTERS) =================
float Car::getX() const {
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return pos.x;
}

float Car::getY() const {
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return pos.y;
}

float Car::getAngle() const {
    b2Rot rot = b2Body_GetRotation(bodyId);
    return b2Rot_GetAngle(rot);
}

float Car::getVelocityX() const {
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return vel.x;
}

float Car::getVelocityY() const {
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return vel.y;
}

float Car::getCurrentSpeed() const {
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return b2Length(vel);
}

void Car::setPosition(float nx, float ny) {
    b2Vec2 currentPos = {nx, ny};
    b2Rot currentRot = b2Body_GetRotation(bodyId);
    b2Body_SetTransform(bodyId, currentPos, currentRot);
}

void Car::setAngle(float angle_rad) {
    b2Vec2 currentPos = b2Body_GetPosition(bodyId);
    b2Rot newRot = b2MakeRot(angle_rad);
    b2Body_SetTransform(bodyId, currentPos, newRot);
}

void Car::setCurrentSpeed(float speed) {
    float angle = getAngle();
    b2Vec2 newVel = { std::cos(angle) * speed, std::sin(angle) * speed };
    b2Body_SetLinearVelocity(bodyId, newVel);
}


void Car::accelerate(float delta_time) {
    if (is_destroyed) return;

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

    float currentSpeed = getCurrentSpeed();
    if (currentSpeed > max_speed) {
        b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
        float scale = max_speed / currentSpeed;
        b2Vec2 clampedVel = { velocity.x * scale, velocity.y * scale };
        b2Body_SetLinearVelocity(bodyId, clampedVel);
    }
}

void Car::brake(float delta_time) {
    (void)delta_time; 

    if (is_destroyed) return;
    b2Body_SetLinearDamping(bodyId, 5.0f); 
}

void Car::turn_left(float delta_time) {
    if (is_destroyed) return;
    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang - (handling * delta_time));
}

void Car::turn_right(float delta_time) {
    if (is_destroyed) return;
    float current_ang = b2Body_GetAngularVelocity(bodyId);
    b2Body_SetAngularVelocity(bodyId, current_ang + (handling * delta_time));
}

void Car::stop() {
    b2Body_SetLinearVelocity(bodyId, {0,0});
    b2Body_SetAngularVelocity(bodyId, 0);
}


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
    if(nitro_amount > 0) nitro_active = true; 
}

void Car::deactivateNitro() { 
    nitro_active = false; 
    b2Body_SetLinearDamping(bodyId, 1.0f);
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