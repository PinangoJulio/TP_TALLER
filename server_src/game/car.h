#ifndef CAR_H
#define CAR_H

#include <string>
#include <box2d/box2d.h>

/*
 * Car: Representa un auto con física (Box2D) y stats.
 * - Se ha integrado el b2BodyId para manejar la posición y movimiento real.
 * - Se mantienen los métodos de la interfaz original para compatibilidad.
 */
class Car {
private:
    // ---- IDENTIFICACIÓN ----
    std::string model_name;
    std::string car_type;

    // ---- STATS DEL AUTO (cargar desde config.yaml) ----
    float max_speed;
    float acceleration_power; // Fuerza de aceleración
    float handling;
    float max_durability;
    float nitro_boost;
    float weight;

    // ---- ESTADO ACTUAL ----
    float current_health;
    float nitro_amount;
    bool nitro_active;
    bool is_destroyed;
    
    // ---- ESTADOS LÓGICOS (Para visualización) ----
    bool drifting;
    bool colliding;
    
    // ---- FÍSICA BOX2D (Lo nuevo) ----
    b2BodyId bodyId; 

public:
    // Constructor actualizado: Recibe el cuerpo físico ya creado
    Car(const std::string& model, const std::string& type, b2BodyId body);

    // ---- CONFIGURAR STATS ----
    void load_stats(float max_spd, float accel, float hand, float durability, float nitro, float wgt);

    // ---- FÍSICA Y MOVIMIENTO (Consultas a Box2D) ----
    float getX() const;
    float getY() const;
    float getAngle() const;
    float getVelocityX() const;
    float getVelocityY() const;
    float getCurrentSpeed() const;
    
    // Getter directo de Box2D (Útil para colisiones avanzadas)
    b2BodyId getBodyId() const { return bodyId; }

    // ---- SETTERS FÍSICOS (Teletransporte/Reset) ----
    void setPosition(float x, float y);
    void setAngle(float angle_rad);
    void setCurrentSpeed(float speed);

    // ---- COMANDOS DE CONTROL ----
    void accelerate(float delta_time);
    void brake(float delta_time);
    void turn_left(float delta_time);
    void turn_right(float delta_time);
    void stop(); 

    // ---- SALUD Y DAÑO ----
    void takeDamage(float damage);
    void apply_collision_damage(float damage) { takeDamage(damage); } // Alias de compatibilidad
    void repair(float amount);
    bool is_alive() const { return !is_destroyed && current_health > 0; }
    bool isDestroyed() const { return is_destroyed; }
    float getHealth() const { return current_health; }

    // ---- NITRO ----
    void activateNitro();
    void deactivateNitro();
    void rechargeNitro(float amount); // <--- ¡AGREGADO! Faltaba esta línea
    float getNitroAmount() const { return nitro_amount; }
    bool isNitroActive() const { return nitro_active; }
    
    // ---- ESTADO (Drift/Colisión) ----
    bool isDrifting() const { return drifting; }
    void setDrifting(bool state) { drifting = state; }
    
    bool isColliding() const { return colliding; }
    void setColliding(bool state) { colliding = state; }

    // ---- RESET ----
    void reset();

    // ---- GETTERS ----
    const std::string& getModelName() const { return model_name; }
    const std::string& getCarType() const { return car_type; }

    ~Car() = default;
};

#endif // CAR_H