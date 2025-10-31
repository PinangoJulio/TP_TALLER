#ifndef SERVER_CAR_H
#define SERVER_CAR_H

#include <cstddef>

/*Representa el estado de un auto controlado por un cliente
 * Identificado con su id.
 */

struct Car {
    int client_id;
    bool nitro_active;
    int nitro_ticks;  // ticks de nitro en uso
    int nitro_duration;

    Car(int id, int max_duration);

    int get_client_id() const { return client_id; }

    bool activate_nitro();

    bool simulate_tick();

    bool is_nitro_active() const { return nitro_active; }
};
#endif  // SERVER_CAR_H
