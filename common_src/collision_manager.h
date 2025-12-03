#ifndef COLLISION_MANAGER_H
#define COLLISION_MANAGER_H

#include <SDL2pp/SDL2pp.hh>
#include <SDL2pp/Surface.hh>
#include <memory>
#include <string>
#include <vector>
#include <cmath>

// Resultado detallado de una consulta de colisión
struct CollisionResult {
    bool is_wall;           // ¿Hay pared en esta posición?
    bool can_move;          // ¿El auto puede moverse aquí?
    int current_level;      // Nivel detectado (0=calle, 1=puente)
    bool is_on_ramp;        // ¿Está en una rampa?
    
    // Datos físicos para rebote (útil para Box2D o física arcade)
    bool should_bounce;     // ¿Debe rebotar?
    float normal_x;         // Normal de la superficie X
    float normal_y;         // Normal de la superficie Y
    
    CollisionResult() 
        : is_wall(false), can_move(true), current_level(0), 
          is_on_ramp(false), should_bounce(false), 
          normal_x(0.0f), normal_y(0.0f) {}
};

class CollisionManager {
private:
    std::unique_ptr<SDL2pp::Surface> surfCamino;
    std::unique_ptr<SDL2pp::Surface> surfPuentes;
    std::unique_ptr<SDL2pp::Surface> surfRampas;

    Uint32 getPixel(SDL2pp::Surface& surface, int x, int y);
    
    // Calcula la normal de la pared muestreando píxeles vecinos
    void calculateSurfaceNormal(int x, int y, int level, float& normal_x, float& normal_y);

public:
    CollisionManager(const std::string& pathCamino, const std::string& pathPuentes,
                     const std::string& pathRampas = "");

    // --- Métodos de consulta individuales ---
    bool isWall(int x, int y, int currentLevel);
    bool hasGroundLevel(int x, int y);
    bool hasBridgeLevel(int x, int y);
    bool isRamp(int x, int y);
    bool isOnBridge(int x, int y); // Alias de hasBridgeLevel
    bool canTransition(int x, int y, int fromLevel, int toLevel);

    // --- Método unificado principal ---
    /**
     * Verifica colisión y lógica de movimiento en una posición específica.
     * @param x Posición actual X
     * @param y Posición actual Y
     * @param current_level Nivel actual (0 o 1)
     * @param next_x Posición futura X
     * @param next_y Posición futura Y
     * @return Estructura con el resultado del análisis
     */
    CollisionResult checkCollision(int x, int y, int current_level, 
                                   int next_x, int next_y);

    // Dimensiones
    int GetWidth() const { return surfCamino ? surfCamino->GetWidth() : 0; }
    int GetHeight() const { return surfCamino ? surfCamino->GetHeight() : 0; }
};

#endif  // COLLISION_MANAGER_H