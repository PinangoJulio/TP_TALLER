#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <memory>
#include <atomic>
#include <string>
#include <utility>
#include "car.h"

class Player {
private:
    // ---- INFORMACIÓN DEL JUGADOR ----
    int id;
    std::string name;
    std::string selected_car_name;
    
    // ---- AUTO ASIGNADO ----
    // Player es DUEÑO de su Car (gestión automática de memoria)
    std::unique_ptr<Car> car;

    // ---- ESTADO EN LA CARRERA ----
    int completed_laps;
    int current_checkpoint;
    int position_in_race;  // 1st, 2nd, 3rd, etc.
    int score;
    bool finished_race;

    // ---- ESTADO DEL JUGADOR ----
    bool disconnected;
    bool is_ready;  // Para el lobby

public:
    explicit Player(int id, const std::string& name)
        : id(id), name(name), 
          selected_car_name(""),
          car(nullptr), 
          completed_laps(0), 
          current_checkpoint(0),
          position_in_race(0), 
          score(0), 
          finished_race(false), 
          disconnected(false), 
          is_ready(false) {
    }

    // ---- GESTIÓN DEL AUTO ----
    
    // Método para transferir ownership del Car al Player
    void setCarOwnership(std::unique_ptr<Car> new_car) { 
        car = std::move(new_car); 
        if (car) {
            selected_car_name = car->getModelName();
        }
    }

    // Método legacy para compatibilidad (NO debería usarse en código nuevo)
    void setCar(Car* new_car) {
        // ⚠️ ADVERTENCIA: No transfiere ownership
        // Solo se mantiene para compatibilidad con código antiguo
        (void)new_car;  // Suppress unused warning
    }

    Car* getCar() { return car.get(); }
    const Car* getCar() const { return car.get(); }

    void setSelectedCar(const std::string& carName) {
        selected_car_name = carName;
    }
    
    std::string getSelectedCar() const {
        return selected_car_name;
    }

    // ---- GETTERS BÁSICOS ----
    int getId() const { return id; }
    const std::string& getName() const { return name; }

    // ---- POSICIÓN (DELEGADO AL CAR) ----
    float getX() const { return car ? car->getX() : 0.0f; }
    float getY() const { return car ? car->getY() : 0.0f; }
    void setPosition(float newX, float newY) {
        if (car)
            car->setPosition(newX, newY);
    }

    float getAngle() const { return car ? car->getAngle() : 0.0f; }
    void setAngle(float newAngle) {
        if (car)
            car->setAngle(newAngle);
    }

    float getSpeed() const { return car ? car->getCurrentSpeed() : 0.0f; }
    void setSpeed(float newSpeed) {
        if (car)
            car->setCurrentSpeed(newSpeed);
    }

    // ---- VUELTAS Y CHECKPOINTS ----
    int getCompletedLaps() const { return completed_laps; }
    void incrementLap() { completed_laps++; }
    void setCompletedLaps(int laps) { completed_laps = laps; }

    int getCurrentCheckpoint() const { return current_checkpoint; }
    void setCheckpoint(int checkpoint) { current_checkpoint = checkpoint; }

    // ---- POSICIÓN EN LA CARRERA ----
    int getPositionInRace() const { return position_in_race; }
    void setPositionInRace(int pos) { position_in_race = pos; }

    // ---- SCORE ----
    int getScore() const { return score; }
    void addScore(int value) { score += value; }
    void setScore(int value) { score = value; }

    // ---- ESTADO DE LA CARRERA ----
    bool isFinished() const { return finished_race; }
    void markAsFinished() { finished_race = true; }

    // ---- ESTADO (DELEGADO AL CAR) ----
    const std::string& getCarType() const {
        static const std::string empty = "None";
        return car ? car->getCarType() : empty;
    }

    bool isDrifting() const { 
        return car ? car->isDrifting() : false; 
    }
    void setDrifting(bool state) {
        if (car)
            car->setDrifting(state);
    }

    bool isColliding() const { 
        return car ? car->isColliding() : false; 
    }
    void setColliding(bool state) {
        if (car)
            car->setColliding(state);
    }

    // ---- CONEXIÓN ----
    bool isDisconnected() const { return disconnected; }
    void disconnect() { disconnected = true; }
    void setDisconnected(bool val) { disconnected = val; }

    // ---- READY (PARA LOBBY) ----
    bool getIsReady() const { return is_ready; }
    void setReady(bool ready) { is_ready = ready; }

    // ---- ALIAS PARA COMPATIBILIDAD ----
    bool isAlive() const { 
        return car ? !car->isDestroyed() : true; 
    }
    
    // Método para compatibilidad con el segundo archivo
    bool is_alive() const { return isAlive(); }
    
    void kill() {
        if (car)
            car->takeDamage(1000.0f);  // Daño mortal
    }

    // ---- RESET PARA NUEVA CARRERA ----
    void resetForNewRace() {
        completed_laps = 0;
        current_checkpoint = 0;
        position_in_race = 0;
        finished_race = false;
        disconnected = false;
        if (car) {
            car->reset();
        }
    }

    // ---- DESTRUCTOR ----
    ~Player() {
        // ✅ unique_ptr libera automáticamente el Car
    }
};

#endif  // PLAYER_H