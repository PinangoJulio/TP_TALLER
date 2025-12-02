#ifndef CAR_H
#define CAR_H

#include <cstddef>
#include <string>
#include <box2d/box2d.h>

/*
 * Car: Representa un auto con física Box2D y stats
 * - Maneja posición, velocidad, ángulo a través de Box2D
 * - Stats específicos del modelo (cargados desde config.yaml)
 * - Salud, nitro, estado
 * - Encapsula el cuerpo de Box2D
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

    // ---- FÍSICA BOX2D ----
    b2BodyId bodyId;       // Cuerpo físico de Box2D

    // ---- ESTADO ----
    bool drifting;         // Si está derrapando
    bool colliding;        // Si está colisionando
    bool is_destroyed;     // Si está destruido

public:
    // ---- CONSTRUCTOR ----
    Car(const std::string& model, const std::string& type, b2BodyId body);

    // ---- CONFIGURAR STATS ----
    void load_stats(float max_spd, float accel, float hand, 
                    float durability, float nitro, float wgt);

    // ---- GESTIÓN DE CUERPO BOX2D ----
    void setBodyId(b2BodyId newBody);
    b2BodyId getBodyId() const { return bodyId; }

    // ---- GETTERS FÍSICOS (DELEGADOS A BOX2D) ----
    float getX() const;
    float getY() const;
    float getAngle() const;
    float getVelocityX() const;
    float getVelocityY() const;
    float getCurrentSpeed() const { return current_speed; }

    // ---- SETTERS FÍSICOS (DELEGADOS A BOX2D) ----
    void setPosition(float x, float y);
    void setAngle(float angle_rad);
    void setCurrentSpeed(float speed);

    // ---- STATS ----
    // NOTA: Estos getters pueden no existir si las variables son privadas
    // Si se necesitan, agregarlos aquí:
     float getMaxSpeed() const { return max_speed; }
     float getAcceleration() const { return acceleration; }
     float getHandling() const { return handling; }
     float getWeight() const { return weight; }

    // ---- SALUD Y DAÑO ----
    float getHealth() const { return current_health; }
    void takeDamage(float damage);
    void apply_collision_damage(float damage) { takeDamage(damage); }
    void repair(float amount);
    bool isDestroyed() const { return is_destroyed; }
    bool is_alive() const { return !is_destroyed && current_health > 0; }

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
    void stop(); 
    void updatePhysics();

    // ---- ESTADO ----
    bool isDrifting() const { return drifting; }
    void setDrifting(bool state) { drifting = state; }

    bool isColliding() const { return colliding; }
    void setColliding(bool state) { colliding = state; }

    // ---- RESET ----
    void reset();

    // ---- GETTERS DE INFORMACIÓN ----
    const std::string& getModelName() const { return model_name; }
    const std::string& getCarType() const { return car_type; }

    // ---- DESTRUCTOR ----
    ~Car() = default;
};

#endif  // CAR_H