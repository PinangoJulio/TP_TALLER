#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <memory>
#include "car.h"

class Player {
private:
    int id;
    std::string name;
    std::string selected_car_name;
    
    std::unique_ptr<Car> car; // Propiedad exclusiva del auto físico

    // Estado de partida
    bool is_ready;
    bool is_finished;
    bool is_disconnected;
    
    // Estado de carrera
    int completed_laps;
    int current_checkpoint;
    int score;

public:
    Player(int id, std::string name) 
        : id(id), name(name), car(nullptr), 
          is_ready(false), is_finished(false), is_disconnected(false),
          completed_laps(0), current_checkpoint(0), score(0) {}

    // ---- GESTIÓN DEL AUTO ----
    void setCarOwnership(std::unique_ptr<Car> newCar) {
        car = std::move(newCar);
    }
    
    Car* getCar() const {
        return car.get();
    }

    void setSelectedCar(const std::string& carName) {
        selected_car_name = carName;
    }
    
    std::string getSelectedCar() const {
        return selected_car_name;
    }

    // ---- ESTADO DE CONEXIÓN ----
    bool isDisconnected() const { return is_disconnected; }
    void setDisconnected(bool val) { is_disconnected = val; }
    void disconnect() { is_disconnected = true; }

    // ---- ESTADO DE JUEGO ----
    void setReady(bool ready) { is_ready = ready; }
    bool getIsReady() const { return is_ready; }
    
    void markAsFinished() { is_finished = true; }
    bool isFinished() const { return is_finished; }
    
    void setCompletedLaps(int laps) { completed_laps = laps; }
    int getCompletedLaps() const { return completed_laps; }

    // ---- GETTERS BÁSICOS ----
    int getId() const { return id; }
    const std::string& getName() const { return name; }
    
    // ---- HELPERS PARA SNAPSHOT (DELEGACIÓN A CAR) ----
    
    const std::string& getCarType() const { 
        if (car) return car->getCarType();
        static const std::string empty = "None";
        return empty;
    }

    bool isDrifting() const { 
        return car ? car->isDrifting() : false; 
    }

    bool isColliding() const { 
        return car ? car->isColliding() : false; 
    }

    float getX() const { return car ? car->getX() : 0.0f; }
    float getY() const { return car ? car->getY() : 0.0f; }
    float getAngle() const { return car ? car->getAngle() : 0.0f; }
    float getSpeed() const { return car ? car->getCurrentSpeed() : 0.0f; }
    bool isAlive() const { return car ? car->is_alive() : false; }
    
    // ---- HELPERS EXTRAS ----
    int getCurrentCheckpoint() const { return current_checkpoint; } 
    int getPositionInRace() const { return 0; } 
    int getScore() const { return score; }

    void resetForNewRace() {
        is_finished = false;
        completed_laps = 0;
        current_checkpoint = 0;
    }
    
    void setPosition(float x, float y) { if(car) car->setPosition(x, y); }
    void setAngle(float angle) { if(car) car->setAngle(angle); }

    ~Player() = default;
};

#endif // PLAYER_H