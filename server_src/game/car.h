#ifndef CAR_H
#define CAR_H

#include <cstddef>
#include <string>

// FORWARD DECLARATIONS (en vez de #include <box2d/box2d.h>)
struct b2BodyId;
struct b2WorldId;

// Solo incluir las constantes (no dependen de Box2D)
#include "physics_constants.h"

/*
 * Car: Representa un auto con física y stats
 */
class Car {
private:
    // ---- IDENTIFICACIÓN ----
    std::string model_name;
    std::string car_type;

    // ---- STATS ----
    float max_speed;
    float acceleration;
    float handling;
    float max_durability;
    float nitro_boost;
    float weight;

    // ---- ESTADO ACTUAL ----
    float current_speed;
    float current_health;
    float nitro_amount;
    bool nitro_active;

    // ---- FÍSICA Y POSICIÓN ----
    float x;
    float y;
    float angle;
    float velocity_x;
    float velocity_y;

    // ---- BOX2D v3 (IDs opacos, no necesitan definición completa) ----
    b2BodyId body_id;
    bool has_physics_body;
    float car_size_px;

    // ---- ESTADO ----
    bool is_drifting;
    bool is_colliding;
    bool is_destroyed;

public:
    // ---- CONSTRUCTOR ----
    Car(const std::string& model, const std::string& type);

    // ---- CONFIGURAR STATS ----
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
    void update(float delta_time);
    void accelerate(float delta_time);
    void brake(float delta_time);
    void turn_left(float delta_time);
    void turn_right(float delta_time);

    // ---- MOVIMIENTO EN 4 DIRECCIONES ----
    void move_up(float delta_time);
    void move_down(float delta_time);
    void move_left(float delta_time);
    void move_right(float delta_time);

    // ---- FÍSICA ----
    void apply_friction(float delta_time);

    // ---- ESTADO ----
    void setDrifting(bool drifting) { is_drifting = drifting; }
    bool isDrifting() const { return is_drifting; }

    void setColliding(bool colliding) { is_colliding = colliding; }
    bool isColliding() const { return is_colliding; }

    // ---- BOX2D v3 ----
    void createPhysicsBody(b2WorldId world_id, float spawn_x_px, float spawn_y_px, float spawn_angle);
    void syncFromPhysics();
    void destroyPhysicsBody(b2WorldId world_id);
    b2BodyId getBodyId() const { return body_id; }
    bool hasPhysicsBody() const { return has_physics_body; }

    // ---- RESET ----
    void reset();

    // ---- GETTERS ----
    const std::string& getModelName() const { return model_name; }
    const std::string& getCarType() const { return car_type; }

    ~Car() = default;
};

#endif  // CAR_H