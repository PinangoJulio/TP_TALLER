#ifndef COLLISION_MANAGER_H
#define COLLISION_MANAGER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <memory>
#include <string>
#include <iostream>

/**
 * CollisionManager - Sistema de colisiones basado en máscaras de imagen
 * 
 * Versión COMÚN para cliente y servidor (usa SDL2 puro, no SDL2pp)
 * 
 * Sistema de niveles:
 * - Nivel 0: Calle (capa "camino.png")
 * - Nivel 1: Puente (capa "puentes.png")
 * - Rampas: Zonas de transición entre niveles (capa "rampas.png")
 */
class CollisionManager {
private:
    SDL_Surface* surfCamino;
    SDL_Surface* surfPuentes;
    SDL_Surface* surfRampas;

    Uint32 getPixel(SDL_Surface* surface, int x, int y);

public:
    CollisionManager(const std::string& pathCamino, 
                     const std::string& pathPuentes,
                     const std::string& pathRampas = "");

    ~CollisionManager();

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
    int GetWidth() const { return surfCamino ? surfCamino->w : 0; }
    int GetHeight() const { return surfCamino ? surfCamino->h : 0; }
};

#endif  // COLLISION_MANAGER_H