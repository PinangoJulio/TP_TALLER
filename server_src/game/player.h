#ifndef PLAYER_H_
#define PLAYER_H_

#include <string>
#include <atomic>
#include "car.h"  // ✅ Player tiene un Car

class Player {
private:
    // ---- INFORMACIÓN DEL JUGADOR ----
    int id;
    std::string name;

    // ---- AUTO ASIGNADO ----
    Car* car;  // ✅ El auto con toda la física

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
        : id(id),
          name(name),
          car(nullptr),
          completed_laps(0),
          current_checkpoint(0),
          position_in_race(0),
          score(0),
          finished_race(false),
          disconnected(false),
          is_ready(false) {}

    // --- Auto ---
    void setCar(Car* new_car) { car = new_car; }
    Car* getCar() { return car; }
    const Car* getCar() const { return car; }

    // Helpers para acceder a propiedades del auto
    const std::string& getSelectedCar() const {
        static const std::string empty = "";
        return car ? car->getModelName() : empty;
    }
    void setSelectedCar(const std::string& /* model */) {
        // No hace nada, el auto se asigna con setCar()
    }

    const std::string& getCarType() const {
        static const std::string empty = "";
        return car ? car->getCarType() : empty;
    }
    void getCarType(const std::string& /* type */) {
        // Typo heredado, no hace nada
    }

    // --- Basic Getters ---
    int getId() const { return id; }
    const std::string& getName() const { return name; }

    // --- Posición (delegado al Car) ---
    float getX() const { return car ? car->getX() : 0.0f; }
    float getY() const { return car ? car->getY() : 0.0f; }
    void setPosition(float newX, float newY) {
        if (car) car->setPosition(newX, newY);
    }

    float getAngle() const { return car ? car->getAngle() : 0.0f; }
    void setAngle(float newAngle) {
        if (car) car->setAngle(newAngle);
    }

    float getSpeed() const { return car ? car->getCurrentSpeed() : 0.0f; }
    void setSpeed(float newSpeed) {
        if (car) car->setCurrentSpeed(newSpeed);
    }

    // --- Vueltas y Checkpoints ---
    int getCompletedLaps() const { return completed_laps; }
    void incrementLap() { completed_laps++; }
    void setCompletedLaps(int laps) { completed_laps = laps; }

    int getCurrentCheckpoint() const { return current_checkpoint; }
    void setCheckpoint(int checkpoint) { current_checkpoint = checkpoint; }

    // --- Posición en la carrera ---
    int getPositionInRace() const { return position_in_race; }
    void setPositionInRace(int pos) { position_in_race = pos; }

    // --- Score ---
    int getScore() const { return score; }
    void addScore(int value) { score += value; }
    void setScore(int value) { score = value; }

    // --- Estado de la carrera ---
    bool isFinished() const { return finished_race; }
    void markAsFinished() { finished_race = true; }

    // --- Estado (delegado al Car) ---
    bool isDrifting() const { return car ? car->isDrifting() : false; }
    void setDrifting(bool state) {
        if (car) car->setDrifting(state);
    }

    bool isColliding() const { return car ? car->isColliding() : false; }
    void setColliding(bool state) {
        if (car) car->setColliding(state);
    }

    // --- Conexión ---
    bool isDisconnected() const { return disconnected; }
    void disconnect() { disconnected = true; }

    // --- Ready (para lobby) ---
    bool getIsReady() const { return is_ready; }
    void setReady(bool ready) { is_ready = ready; }

    // --- Alias para compatibilidad ---
    bool isAlive() const { return car ? !car->isDestroyed() : true; }
    void kill() {
        if (car) car->takeDamage(1000.0f);  // Daño mortal
    }

    // --- Reset para nueva carrera ---
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

    ~Player() {
        // NO eliminar el car aquí, es manejado por GameLoop
    }
};

#endif // PLAYER_H_
