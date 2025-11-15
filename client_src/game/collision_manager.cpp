#include "collision_manager.h"
#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL_image.h>
#include <iostream>

CollisionManager::CollisionManager(const std::string& pathCamino, 
                                 const std::string& pathPuentes) {
    try {
        // usando sdl y no el wrapper
        SDL_Surface* rawCamino = IMG_Load(pathCamino.c_str());
        SDL_Surface* rawPuentes = IMG_Load(pathPuentes.c_str());

        if (!rawCamino || !rawPuentes) {
            throw std::runtime_error("Error al cargar imágenes: " + std::string(IMG_GetError()));
        }

        // Envolver en SDL2pp::Surface
        surfCamino = std::make_unique<SDL2pp::Surface>(rawCamino);
        surfPuentes = std::make_unique<SDL2pp::Surface>(rawPuentes);

        std::cout << "CollisionManager: Capas cargadas OK." << std::endl;
        std::cout << " - Camino: " << surfCamino->GetWidth() << "x" << surfCamino->GetHeight() << std::endl;
        std::cout << " - Puentes: " << surfPuentes->GetWidth() << "x" << surfPuentes->GetHeight() << std::endl;

    } catch (const std::exception& e) {
        throw std::runtime_error("Error al cargar capas de colisión: " + std::string(e.what()));
    }
}

Uint32 CollisionManager::getPixel(SDL2pp::Surface& surface, int x, int y) {
    if (x < 0 || x >= surface.GetWidth() || y < 0 || y >= surface.GetHeight()) {
        return 0;
    }

    SDL_Surface* rawSurf = surface.Get();
    int bpp = rawSurf->format->BytesPerPixel;
    Uint8 *p = (Uint8 *)rawSurf->pixels + y * rawSurf->pitch + x * bpp;

    switch (bpp) {
        case 1:
            return *p;
        case 2:
            return *(Uint16 *)p;
        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
        case 4:
            return *(Uint32 *)p;
        default:
            return 0;
    }
}

bool CollisionManager::hasGroundLevel(int x, int y) {
    SDL_Surface* rawCamino = surfCamino->Get();
    SDL_LockSurface(rawCamino);
    
    Uint32 pixelCamino = getPixel(*surfCamino, x, y);
    SDL_Color rgbCamino;
    SDL_GetRGB(pixelCamino, rawCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);
    
    SDL_UnlockSurface(rawCamino);
    
    // Blanco = hay camino en nivel 0
    return rgbCamino.r > 128;
}

bool CollisionManager::hasBridgeLevel(int x, int y) {
    SDL_Surface* rawPuentes = surfPuentes->Get();
    SDL_LockSurface(rawPuentes);
    
    Uint32 pixelPuentes = getPixel(*surfPuentes, x, y);
    SDL_Color rgbPuentes;
    SDL_GetRGB(pixelPuentes, rawPuentes->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);
    
    SDL_UnlockSurface(rawPuentes);
    
    // Blanco = hay puente en nivel 1
    return rgbPuentes.r > 128;
}

bool CollisionManager::isWall(int x, int y, int currentLevel) {
    SDL_Surface* rawCamino = surfCamino->Get();
    SDL_Surface* rawPuentes = surfPuentes->Get();

    SDL_LockSurface(rawCamino);
    SDL_LockSurface(rawPuentes);

    Uint32 pixelCamino = getPixel(*surfCamino, x, y);
    Uint32 pixelPuentes = getPixel(*surfPuentes, x, y);

    SDL_Color rgbCamino, rgbPuentes;
    SDL_GetRGB(pixelCamino, rawCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);
    SDL_GetRGB(pixelPuentes, rawPuentes->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);

    SDL_UnlockSurface(rawCamino);
    SDL_UnlockSurface(rawPuentes);

    bool isCamino = rgbCamino.r > 128;
    bool isPuente = rgbPuentes.r > 128;

    // Lógica según el nivel actual
    if (currentLevel == 0) {
       // Nivel calle: solo puedo moverme por camino
        return !isCamino;
    } else {
        // Nivel puente: solo puedo moverme por puente, a chequear porue a veces se sale del puente y cae en el camino
        return !isPuente;
    }
}

bool CollisionManager::isOnBridge(int x, int y) {
    return hasBridgeLevel(x, y);
}