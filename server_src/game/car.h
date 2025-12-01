#ifndef CAR_H
#define CAR_H

#include <string>
#include <box2d/box2d.h>

class Car {
private:
    // ---- IDENTIFICACIÓN ----
    std::string model_name;
    std::string car_type;

    // ---- STATS ----
    float max_speed;
    float acceleration_power;
    float handling;
    float max_durability;
    float nitro_boost;
    float weight;

    // ---- ESTADO ----
    float current_health;
    float nitro_amount;
    bool nitro_active;
    bool is_destroyed;
    bool drifting;
    bool colliding;
    
    // ---- FÍSICA ----
    b2BodyId bodyId; 

public:
    Car(const std::string& model, const std::string& type, b2BodyId body);

    // Configuración
    void load_stats(float max_spd, float accel, float hand, float durability, float nitro, float wgt);

    void setBodyId(b2BodyId newBody);

    // Getters Físicos
    float getX() const;
    float getY() const;
    float getAngle() const;
    float getVelocityX() const;
    float getVelocityY() const;
    float getCurrentSpeed() const;
    b2BodyId getBodyId() const { return bodyId; }

    // Setters Físicos
    void setPosition(float x, float y);
    void setAngle(float angle_rad);
    void setCurrentSpeed(float speed);

    // Controles
    void accelerate(float delta_time);
    void brake(float delta_time);
    void turn_left(float delta_time);
    void turn_right(float delta_time);
    void stop(); 

    // Lógica
    void takeDamage(float damage);
    void apply_collision_damage(float damage) { takeDamage(damage); }
    void repair(float amount);
    void activateNitro();
    void deactivateNitro();
    void rechargeNitro(float amount); 
    void reset();

    // Estado
    float getHealth() const { return current_health; }
    bool is_alive() const { return !is_destroyed && current_health > 0; }
    bool isDestroyed() const { return is_destroyed; }
    float getNitroAmount() const { return nitro_amount; }
    bool isNitroActive() const { return nitro_active; }
    
    bool isDrifting() const { return drifting; }
    void setDrifting(bool state) { drifting = state; }
    
    bool isColliding() const { return colliding; }
    void setColliding(bool state) { colliding = state; }

    const std::string& getModelName() const { return model_name; }
    const std::string& getCarType() const { return car_type; }

    ~Car() = default;
};

#endif // CAR_H