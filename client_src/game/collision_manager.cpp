#include "collision_manager.h"

#include <SDL_image.h>

#include <SDL2pp/SDL2pp.hh>
#include <iostream>

CollisionManager::CollisionManager(const std::string& pathCamino, const std::string& pathPuentes,
                                   const std::string& pathRampas) {
    try {
        // Cargar capas principales
        SDL_Surface* layerCamino = IMG_Load(pathCamino.c_str());
        SDL_Surface* layerPuente = IMG_Load(pathPuentes.c_str());

        if (!layerCamino || !layerPuente) {
            throw std::runtime_error("Error al cargar imágenes: " + std::string(IMG_GetError()));
        }

        surfCamino = std::make_unique<SDL2pp::Surface>(layerCamino);
        surfPuentes = std::make_unique<SDL2pp::Surface>(layerPuente);

        // Cargar capa de rampas
        if (!pathRampas.empty()) {
            SDL_Surface* layerRampas = IMG_Load(pathRampas.c_str());
            if (layerRampas) {
                surfRampas = std::make_unique<SDL2pp::Surface>(layerRampas);
                std::cout << "CollisionManager: Capa de rampas cargada." << std::endl;
            }
        }

        std::cout << "CollisionManager: Capas cargadas OK." << std::endl;

    } catch (const std::exception& e) {
        throw std::runtime_error("Error al cargar capas de colisión: " + std::string(e.what()));
    }
}

Uint32 CollisionManager::getPixel(SDL2pp::Surface& surface, int x, int y) {
    // Verifico que el pixel que estoy chequeando efectivamente pertenezca a la img
    if (x < 0 || x >= surface.GetWidth() || y < 0 || y >= surface.GetHeight()) {
        return 0;
    }

    SDL_Surface* rawSurf = surface.Get();
    int bpp = rawSurf->format->BytesPerPixel;
    Uint8* p = (Uint8*)rawSurf->pixels + y * rawSurf->pitch + x * bpp;

    // Feo feo
    switch (bpp) {
    case 1:
        return *p;
    case 2:
        return *(Uint16*)p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4:
        return *(Uint32*)p;
    default:
        return 0;
    }
}

bool CollisionManager::hasGroundLevel(int x, int y) {
    SDL_Surface* layerCamino = surfCamino->Get();
    SDL_LockSurface(layerCamino);

    Uint32 pixelCamino = getPixel(*surfCamino, x, y);
    SDL_Color rgbCamino;
    SDL_GetRGB(pixelCamino, layerCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);

    SDL_UnlockSurface(layerCamino);

    return rgbCamino.r > 128;
}

bool CollisionManager::hasBridgeLevel(int x, int y) {
    SDL_Surface* layerPuente = surfPuentes->Get();
    SDL_LockSurface(layerPuente);

    Uint32 pixelPuentes = getPixel(*surfPuentes, x, y);
    SDL_Color rgbPuentes;
    SDL_GetRGB(pixelPuentes, layerPuente->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);

    SDL_UnlockSurface(layerPuente);

    return rgbPuentes.r > 128;
}

bool CollisionManager::isRamp(int x, int y) {
    if (!surfRampas) {
        // Si no hay máscara de rampas, permito transiciones en cualquier lugar
        // donde se superpongan camino y puente, no es lo ideal porque toma una especie
        // de vista aerea y el carro pasa entre puente y camino por cualquier lado básicamente
        return hasGroundLevel(x, y) && hasBridgeLevel(x, y);
    }

    SDL_Surface* layerRampas = surfRampas->Get();
    SDL_LockSurface(layerRampas);

    Uint32 pixelRampas = getPixel(*surfRampas, x, y);
    SDL_Color rgbRampas;
    SDL_GetRGB(pixelRampas, layerRampas->format, &rgbRampas.r, &rgbRampas.g, &rgbRampas.b);

    SDL_UnlockSurface(layerRampas);

    // Blanco = es una rampa
    return rgbRampas.r > 128;
}

bool CollisionManager::canTransition(int x, int y, int fromLevel, int toLevel) {
    // Verificar que estemos en una zona de rampa válida
    bool hasGround = hasGroundLevel(x, y);
    bool hasBridge = hasBridgeLevel(x, y);
    bool isRampZone = isRamp(x, y);

    if (fromLevel == 0 && toLevel == 1) {
        // Subiendo de calle a puente
        return isRampZone && hasBridge;
    } else if (fromLevel == 1 && toLevel == 0) {
        // Bajando de puente a calle
        return isRampZone && hasGround;
    }

    return false;
}

bool CollisionManager::isWall(int x, int y, int currentLevel) {
    SDL_Surface* layerCamino = surfCamino->Get();
    SDL_Surface* layerPuente = surfPuentes->Get();

    SDL_LockSurface(layerCamino);
    SDL_LockSurface(layerPuente);

    Uint32 pixelCamino = getPixel(*surfCamino, x, y);
    Uint32 pixelPuentes = getPixel(*surfPuentes, x, y);

    SDL_Color rgbCamino, rgbPuentes;
    SDL_GetRGB(pixelCamino, layerCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);
    SDL_GetRGB(pixelPuentes, layerPuente->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);

    SDL_UnlockSurface(layerCamino);
    SDL_UnlockSurface(layerPuente);

    bool isCamino = rgbCamino.r > 128;
    bool isPuente = rgbPuentes.r > 128;

    if (currentLevel == 0) {
        // Nivel camino, solo puedo moverme por camino
        return !isCamino;
    } else {
        // Nivel puente, solo puedo moverme por puente
        return !isPuente;
    }
}

bool CollisionManager::isOnBridge(int x, int y) {
    return hasBridgeLevel(x, y);
}
