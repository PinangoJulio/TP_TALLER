#ifndef CAR_H
#define CAR_H

#include <cstddef>
#include <string>

/* Representa el estado de un auto controlado por un cliente
 * Identificado con su id.
 */
class Car {
private:
    std::string name;
    std::string type;
    float speed;
    float acceleration;
    float handling;
    float durability;

    float x;
    float y;
    float angle;

public:
    Car(const std::string& name, const std::string& type)
        : name(name), type(type), speed(0), acceleration(0), handling(0),
          durability(0), x(0), y(0), angle(0) {}

    // getters / setters
    void setPosition(float nx, float ny) { x = nx; y = ny; }
    float getX() const { return x; }
    float getY() const { return y; }

    float getAngle() const { return angle; }
    void setAngle(float angle_) { this->angle = angle_; }

    float getSpeed() const { return speed; }
    void setSpeed(float speed_) { this->speed = speed_; }

    float getAcceleration() const { return acceleration; }
    void setAcceleration(float acceleration_) { this->acceleration = acceleration_; }

    float getHandling() const { return handling; }
    void setHandling(float handling_) { this->handling = handling_; }

    float getDurability() const { return durability; }
    void setDurability(float durability_) { this->durability = durability_; }

};

#endif // CAR_H