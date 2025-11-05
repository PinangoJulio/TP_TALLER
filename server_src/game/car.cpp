#include "car.h"  

Car::Car(int id, int max_duration):
        client_id(id), nitro_active(false), nitro_ticks(0), nitro_duration(max_duration) {}

bool Car::activate_nitro() {
    if (nitro_active)
        return false;
    nitro_active = true;
    nitro_ticks = 0;
    return true;
}

bool Car::simulate_tick() {
    if (!nitro_active)
        return false;  // nitro no activo
    nitro_ticks++;
    if (nitro_ticks >= nitro_duration) {
        nitro_active = false;
        nitro_ticks = 0;
        return true;  // nitro expiró
    }
    return false;
}