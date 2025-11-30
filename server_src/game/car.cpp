#include "car.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// ==========================================================
// CONSTRUCTOR
// ==========================================================

Car::Car(const std::string& model, const std::string& type)
    : model_name(model), car_type(type), max_speed(100.0f), acceleration(50.0f), handling(1.0f),
      max_durability(100.0f), nitro_boost(1.5f), weight(1000.0f), current_speed(0.0f),
      current_health(100.0f), nitro_amount(100.0f), nitro_active(false), x(0.0f), y(0.0f),
      angle(0.0f), velocity_x(0.0f), velocity_y(0.0f), is_drifting(false), is_colliding(false),
      is_destroyed(false) {
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
    if (is_destroyed)
        return;

    // recalcular componentes de velocidad por si no se actualizan en accelerate/brake
    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);

    // integrar posición
    x += velocity_x * delta_time;
    y += velocity_y * delta_time;
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

    current_speed = std::min(max_speed, current_speed + effective_acceleration * delta_time);

    update(delta_time);
}

void Car::brake(float delta_time) {
    if (is_destroyed)
        return;

    float brake_force = acceleration * 2.0f;  // Frenar es más rápido que acelerar
    current_speed = std::max(0.0f, current_speed - brake_force * delta_time);

    velocity_x = current_speed * std::cos(angle);
    velocity_y = current_speed * std::sin(angle);
}

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
