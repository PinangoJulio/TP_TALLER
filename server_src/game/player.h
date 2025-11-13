#ifndef PLAYER_H_
#define PLAYER_H_

#include <string>
#include <atomic>

class Player {
private:
    int id;
    std::string name;
    std::string selectedCar;
    std::string car_type;
    std::string city;

    float x;
    float y;
    float angle;
    float speed;

    int completed_laps;
    int score;
    bool finished_race;
    bool drifting;
    bool colliding;
    bool disconnected;
    bool alive;

public:
    explicit Player(int id, const std::string& name)
        : id(id),
          name(name),
          x(0),
          y(0),
          angle(0),
          speed(0),
          completed_laps(0),
          score(0),
          finished_race(false),
          drifting(false),
          colliding(false),
          disconnected(false),
          alive(true) {}

    // --- Basic Getters and Setters ---
    const std::string& getSelectedCar() const { return selectedCar; }
    void setSelectedCar(const std::string& car) { selectedCar = car; }

    const std::string& getCarType() const { return selectedCar; }
    void getCarType(const std::string& car) { selectedCar = car; }

    const std::string& getCity() const { return city; }
    void setCity(const std::string& newCity) { city = newCity; }

    int getId() const { return id; }
    const std::string& getName() const { return name; }

    float getX() const { return x; }
    float getY() const { return y; }
    void setPosition(float newX, float newY) { x = newX; y = newY; }

    float getAngle() const { return angle; }
    void setAngle(float newAngle) { angle = newAngle; }

    float getSpeed() const { return speed; }
    void setSpeed(float newSpeed) { speed = newSpeed; }

    int getCompletedLaps() const { return completed_laps; }
    void incrementLap() { completed_laps++; }

    int getScore() const { return score; }
    void addScore(int value) { score += value; }

    bool isFinished() const { return finished_race; }
    void markAsFinished() { finished_race = true; }

    bool isDrifting() const { return drifting; }
    void setDrifting(bool state) { drifting = state; }

    bool isColliding() const { return colliding; }
    void setColliding(bool state) { colliding = state; }

    bool isDisconnected() const { return disconnected; }
    void disconnect() { disconnected = true; }

    bool isAlive() const { return alive; }
    void kill() { alive = false; }

    // --- Game logic updates ---

    void resetForNewRace() {
        x = y = speed = 0;
        completed_laps = 0;
        finished_race = false;
        drifting = false;
        colliding = false;
        disconnected = false;
        alive = true;
    }

    ~Player() = default;

    Player(const Player&) = default;
    Player& operator=(const Player&) = default;
    Player(Player&&) = default;
    Player& operator=(Player&&) = default;
};

#endif // PLAYER_H_
