#ifndef COLLISION_MANAGER_H
#define COLLISION_MANAGER_H

#include <SDL2pp/SDL2pp.hh>
#include <SDL2pp/Surface.hh>
#include <string>
#include <memory>

class CollisionManager {
private:
    std::unique_ptr<SDL2pp::Surface> surfCamino;
    std::unique_ptr<SDL2pp::Surface> surfPuentes;

    
    Uint32 getPixel(SDL2pp::Surface& surface, int x, int y);

public:
  
    CollisionManager(const std::string& pathCamino, 
                     const std::string& pathPuentes);

    // Chequea si hay una pared en (x, y) considerando el nivel actual
    bool isWall(int x, int y, int currentLevel);
    
    // Chequea si el jugador está SOBRE un puente
    bool isOnBridge(int x, int y);
    
    // Determina si hay camino transitable en nivel 0
    bool hasGroundLevel(int x, int y);
    
    // Determina si hay puente transitable en nivel 1
    bool hasBridgeLevel(int x, int y);
};

#endif // COLLISION_MANAGER_H