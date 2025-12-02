#include "collision_manager.h"
#include <stdexcept>
#include <algorithm>

CollisionManager::CollisionManager(const std::string& pathCamino, 
                                   const std::string& pathPuentes,
                                   const std::string& pathRampas) 
    : surfCamino(nullptr), surfPuentes(nullptr), surfRampas(nullptr) {
    
    // Cargar capa de camino (obligatoria)
    surfCamino = IMG_Load(pathCamino.c_str());
    if (!surfCamino) {
        throw std::runtime_error("Error al cargar imagen de camino: " + 
                                 std::string(IMG_GetError()));
    }

    // Cargar capa de puentes (obligatoria)
    surfPuentes = IMG_Load(pathPuentes.c_str());
    if (!surfPuentes) {
        SDL_FreeSurface(surfCamino);
        throw std::runtime_error("Error al cargar imagen de puentes: " + 
                                 std::string(IMG_GetError()));
    }

    // Cargar capa de rampas (opcional)
    if (!pathRampas.empty()) {
        surfRampas = IMG_Load(pathRampas.c_str());
        if (surfRampas) {
            std::cout << "[CollisionManager] ✅ Capa de rampas cargada" << std::endl;
        } else {
            std::cout << "[CollisionManager] ⚠️  Capa de rampas no encontrada, "
                      << "se permitirán transiciones en zonas superpuestas" << std::endl;
        }
    }

    std::cout << "[CollisionManager] ✅ Capas cargadas: " 
              << surfCamino->w << "x" << surfCamino->h << std::endl;
}

CollisionManager::~CollisionManager() {
    if (surfCamino) SDL_FreeSurface(surfCamino);
    if (surfPuentes) SDL_FreeSurface(surfPuentes);
    if (surfRampas) SDL_FreeSurface(surfRampas);
}

Uint32 CollisionManager::getPixel(SDL_Surface* surface, int x, int y) {
    if (!surface) return 0;
    
    // Verificar límites
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        return 0;
    }

    int bpp = surface->format->BytesPerPixel;
    Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

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
    SDL_LockSurface(surfCamino);
    
    Uint32 pixelCamino = getPixel(surfCamino, x, y);
    SDL_Color rgbCamino;
    SDL_GetRGB(pixelCamino, surfCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);
    
    SDL_UnlockSurface(surfCamino);
    
    // Blanco (>128) = hay camino
    return rgbCamino.r > 128;
}

bool CollisionManager::hasBridgeLevel(int x, int y) {
    SDL_LockSurface(surfPuentes);
    
    Uint32 pixelPuentes = getPixel(surfPuentes, x, y);
    SDL_Color rgbPuentes;
    SDL_GetRGB(pixelPuentes, surfPuentes->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);
    
    SDL_UnlockSurface(surfPuentes);
    
    // Blanco (>128) = hay puente
    return rgbPuentes.r > 128;
}

bool CollisionManager::isRamp(int x, int y) {
    if (!surfRampas) {
        // Si no hay máscara de rampas, permitir transiciones donde se superpongan capas
        return hasGroundLevel(x, y) && hasBridgeLevel(x, y);
    }

    SDL_LockSurface(surfRampas);
    
    Uint32 pixelRampas = getPixel(surfRampas, x, y);
    SDL_Color rgbRampas;
    SDL_GetRGB(pixelRampas, surfRampas->format, &rgbRampas.r, &rgbRampas.g, &rgbRampas.b);
    
    SDL_UnlockSurface(surfRampas);
    
    // Blanco (>128) = es una rampa
    return rgbRampas.r > 128;
}

bool CollisionManager::canTransition(int x, int y, int fromLevel, int toLevel) {
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
    SDL_LockSurface(surfCamino);
    SDL_LockSurface(surfPuentes);

    Uint32 pixelCamino = getPixel(surfCamino, x, y);
    Uint32 pixelPuentes = getPixel(surfPuentes, x, y);

    SDL_Color rgbCamino, rgbPuentes;
    SDL_GetRGB(pixelCamino, surfCamino->format, &rgbCamino.r, &rgbCamino.g, &rgbCamino.b);
    SDL_GetRGB(pixelPuentes, surfPuentes->format, &rgbPuentes.r, &rgbPuentes.g, &rgbPuentes.b);

    SDL_UnlockSurface(surfCamino);
    SDL_UnlockSurface(surfPuentes);

    bool isCamino = rgbCamino.r > 128;
    bool isPuente = rgbPuentes.r > 128;

    if (currentLevel == 0) {
        // Nivel camino: solo puedo moverme por camino
        return !isCamino;
    } else {
        // Nivel puente: solo puedo moverme por puente
        return !isPuente;
    }
}

bool CollisionManager::isOnBridge(int x, int y) {
    return hasBridgeLevel(x, y);
}