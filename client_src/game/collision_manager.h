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
    std::unique_ptr<SDL2pp::Surface> surfRampas;  // NUEVA: máscara de rampas
    
    Uint32 getPixel(SDL2pp::Surface& surface, int x, int y);

public:
    
    CollisionManager(const std::string& pathCamino, 
                     const std::string& pathPuentes,
                     const std::string& pathRampas = "");

    // Chequea si hay una pared en (x, y) considerando el nivel actual
    bool isWall(int x, int y, int currentLevel);
    
    // Chequea si el jugador está sobre un puente
    bool isOnBridge(int x, int y);
    
    // Determina si hay camino transitable en nivel 0
    bool hasGroundLevel(int x, int y);
    
    // Determina si hay puente transitable en nivel 1
    bool hasBridgeLevel(int x, int y);
    
    // Determina si la posición es una rampa (zona de transición)
    bool isRamp(int x, int y);
    
    // Verifica si la transición de nivel es válida en esta posición
    bool canTransition(int x, int y, int fromLevel, int toLevel);

    // Devuelve las dimensiones de la máscara de colisión principal (camino)
    int GetWidth() const { return surfCamino ? surfCamino->GetWidth() : 0; }
    int GetHeight() const { return surfCamino ? surfCamino->GetHeight() : 0; }
};

#endif // COLLISION_MANAGER_H