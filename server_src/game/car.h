#ifndef CAR_H
#define CAR_H

#include <cstddef>
#include <string>

/*
 * Car: Representa un auto con física y stats
 * - Maneja posición, velocidad, ángulo (física)
 * - Stats específicos del modelo (cargados desde config.yaml)
 * - Salud, nitro, estado
 * - Encapsula el cuerpo de Box2D cuando se implemente
 */
class Car {
private:
    // ---- IDENTIFICACIÓN ----
    std::string model_name;  // Ej: "Leyenda Urbana", "Stallion GT"
    std::string car_type;    // Ej: "classic", "sport", "muscle"

    // ---- STATS DEL AUTO (cargar desde config.yaml) ----
    float max_speed;       // Velocidad máxima
    float acceleration;    // Aceleración
    float handling;        // Manejo (giro)
    float max_durability;  // Durabilidad máxima (HP)
    float nitro_boost;     // Multiplicador de nitro
    float weight;          // Peso (afecta física)

    // ---- ESTADO ACTUAL ----
    float current_speed;   // Velocidad actual
    float current_health;  // Salud actual
    float nitro_amount;    // Cantidad de nitro restante (0-100)
    bool nitro_active;     // Si está usando nitro

    // ---- FÍSICA Y POSICIÓN ----
    float x;           // Posición X
    float y;           // Posición Y
    float angle;       // Ángulo de rotación (radianes)
    float velocity_x;  // Velocidad en X
    float velocity_y;  // Velocidad en Y

    // ---- BOX2D (cuando se implemente) ----
    // b2Body* body;             // Cuerpo de Box2D

    // ---- ESTADO ----
    bool is_drifting;
    bool is_colliding;
    bool is_destroyed;

public:
    // ---- CONSTRUCTOR ----
    Car(const std::string& model, const std::string& type);

    // ---- CONFIGURAR STATS (desde config.yaml) ----
    void load_stats(float max_spd, float accel, float hand, float durability, float nitro,
                    float wgt);

    // ---- FÍSICA Y MOVIMIENTO ----
    void setPosition(float nx, float ny) {
        x = nx;
        y = ny;
    }
    float getX() const { return x; }
    float getY() const { return y; }

    void setVelocity(float vx, float vy) {
        velocity_x = vx;
        velocity_y = vy;
    }
    float getVelocityX() const { return velocity_x; }
    float getVelocityY() const { return velocity_y; }

    float getAngle() const { return angle; }
    void setAngle(float angle_) { angle = angle_; }

    float getCurrentSpeed() const { return current_speed; }
    void setCurrentSpeed(float speed) { current_speed = speed; }

    // ---- STATS ----
    float getMaxSpeed() const { return max_speed; }
    float getAcceleration() const { return acceleration; }
    float getHandling() const { return handling; }
    float getWeight() const { return weight; }

    // ---- SALUD Y DAÑO ----
    float getHealth() const { return current_health; }
    void takeDamage(float damage);
    void repair(float amount);
    bool isDestroyed() const { return is_destroyed; }

    // ---- NITRO ----
    float getNitroAmount() const { return nitro_amount; }
    bool isNitroActive() const { return nitro_active; }
    void activateNitro();
    void deactivateNitro();
    void rechargeNitro(float amount);

    // ---- COMANDOS DE CONTROL ----
    void accelerate(float delta_time);
    void brake(float delta_time);
    void turn_left(float delta_time);
    void turn_right(float delta_time);

    // ---- ESTADO ----
    void setDrifting(bool drifting) { is_drifting = drifting; }
    bool isDrifting() const { return is_drifting; }

    void setColliding(bool colliding) { is_colliding = colliding; }
    bool isColliding() const { return is_colliding; }

    // ---- RESET ----
    void reset();

    // ---- GETTERS ----
    const std::string& getModelName() const { return model_name; }
    const std::string& getCarType() const { return car_type; }

    ~Car() = default;
};

#endif  // CAR_H
