#include "collision_manager.h"
#include <SDL_image.h>
#include <iostream>
#include <cmath>

CollisionManager::CollisionManager(const std::string& pathCamino, 
                                   const std::string& pathPuentes,
                                   const std::string& pathRampas) {
    try {
        SDL_Surface* layerCamino = IMG_Load(pathCamino.c_str());
        SDL_Surface* layerPuente = IMG_Load(pathPuentes.c_str());

        if (!layerCamino || !layerPuente) {
            throw std::runtime_error("Error al cargar imágenes: " + std::string(IMG_GetError()));
        }

        surfCamino = std::make_unique<SDL2pp::Surface>(layerCamino);
        surfPuentes = std::make_unique<SDL2pp::Surface>(layerPuente);

        if (!pathRampas.empty()) {
            SDL_Surface* layerRampas = IMG_Load(pathRampas.c_str());
            if (layerRampas) {
                surfRampas = std::make_unique<SDL2pp::Surface>(layerRampas);
                std::cout << "[CollisionManager] Capa de rampas cargada." << std::endl;
            }
        } else {
            std::cout << "[CollisionManager] Advertencia: Sin capa de rampas." << std::endl;
        }

        std::cout << "[CollisionManager] Inicializado correctamente (" 
                  << GetWidth() << "x" << GetHeight() << ")" << std::endl;

    } catch (const std::exception& e) {
        throw std::runtime_error("Error fatal en CollisionManager: " + std::string(e.what()));
    }
}

Uint32 CollisionManager::getPixel(SDL2pp::Surface& surface, int x, int y) {
    if (x < 0 || x >= surface.GetWidth() || y < 0 || y >= surface.GetHeight()) {
        return 0;
    }

    SDL_Surface* rawSurf = surface.Get();
    int bpp = rawSurf->format->BytesPerPixel;
    Uint8* p = (Uint8*)rawSurf->pixels + y * rawSurf->pitch + x * bpp;

    switch (bpp) {
    case 1: return *p;
    case 2: return *(Uint16*)p;
    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
    case 4: return *(Uint32*)p;
    default: return 0;
    }
}

bool CollisionManager::hasGroundLevel(int x, int y) {
    if (!surfCamino) return false;
    SDL_Surface* s = surfCamino->Get();
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);
    
    Uint32 pixel = getPixel(*surfCamino, x, y);
    SDL_Color rgb;
    SDL_GetRGB(pixel, s->format, &rgb.r, &rgb.g, &rgb.b);
    
    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);
    
    // Asumimos que blanco/claro es camino transitable
    return rgb.r > 128;
}

bool CollisionManager::hasBridgeLevel(int x, int y) {
    if (!surfPuentes) return false;
    SDL_Surface* s = surfPuentes->Get();
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);

    Uint32 pixel = getPixel(*surfPuentes, x, y);
    SDL_Color rgb;
    SDL_GetRGB(pixel, s->format, &rgb.r, &rgb.g, &rgb.b);

    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);

    return rgb.r > 128;
}

bool CollisionManager::isRamp(int x, int y) {
    if (!surfRampas) {
        return hasGroundLevel(x, y) && hasBridgeLevel(x, y);
    }
    
    SDL_Surface* s = surfRampas->Get();
    if (SDL_MUSTLOCK(s)) SDL_LockSurface(s);

    Uint32 pixel = getPixel(*surfRampas, x, y);
    SDL_Color rgb;
    SDL_GetRGB(pixel, s->format, &rgb.r, &rgb.g, &rgb.b);

    if (SDL_MUSTLOCK(s)) SDL_UnlockSurface(s);

    return rgb.r > 128;
}

bool CollisionManager::isWall(int x, int y, int currentLevel) {
    // Si estamos fuera del mapa, es pared
    if (x < 0 || x >= GetWidth() || y < 0 || y >= GetHeight()) {
        return true;
    }

    if (currentLevel == 0) {
        // En nivel suelo (0), si NO hay pixel de camino, es pared
        return !hasGroundLevel(x, y);
    } else {
        // En nivel puente (1), si NO hay pixel de puente, es pared (caída o límite)
        return !hasBridgeLevel(x, y);
    }
}

bool CollisionManager::isOnBridge(int x, int y) {
    return hasBridgeLevel(x, y);
}

bool CollisionManager::canTransition(int x, int y, int fromLevel, int toLevel) {
    bool ramp = isRamp(x, y);
    bool ground = hasGroundLevel(x, y);
    bool bridge = hasBridgeLevel(x, y);

    if (fromLevel == 0 && toLevel == 1) {
        // Subir: Necesito estar en rampa y que exista el puente destino
        return ramp && bridge;
    } else if (fromLevel == 1 && toLevel == 0) {
        // Bajar: Necesito estar en rampa y que exista suelo destino
        return ramp && ground;
    }
    return false;
}

// Lógica de cálculo de normales para rebote
void CollisionManager::calculateSurfaceNormal(int x, int y, int level, float& normal_x, float& normal_y) {
    const int RADIUS = 2; // Radio de búsqueda alrededor del punto de impacto
    float sum_x = 0;
    float sum_y = 0;
    int count = 0;

    // Buscamos píxeles sólidos alrededor. La normal será el promedio inverso de estos vectores.
    for (int dy = -RADIUS; dy <= RADIUS; ++dy) {
        for (int dx = -RADIUS; dx <= RADIUS; ++dx) {
            if (dx == 0 && dy == 0) continue;

            // Si el vecino es pared, contribuye al vector de "dónde está la pared"
            if (isWall(x + dx, y + dy, level)) {
                sum_x += dx;
                sum_y += dy;
                count++;
            }
        }
    }

    if (count > 0) {
        // La normal es opuesta a la dirección promedio de la pared
        normal_x = -sum_x;
        normal_y = -sum_y;
        
        // Normalizar
        float length = std::sqrt(normal_x * normal_x + normal_y * normal_y);
        if (length > 0.001f) {
            normal_x /= length;
            normal_y /= length;
        }
    } else {
        // Fallback: si no detectamos pared alrededor pero isWall dio true (raro),
        // devolvemos una normal genérica hacia el centro del mapa para empujar al jugador dentro.
        float to_center_x = (GetWidth() / 2.0f) - x;
        float to_center_y = (GetHeight() / 2.0f) - y;
        float length = std::sqrt(to_center_x * to_center_x + to_center_y * to_center_y);
        if (length > 0) {
            normal_x = to_center_x / length;
            normal_y = to_center_y / length;
        }
    }
}

CollisionResult CollisionManager::checkCollision(int x, int y, int current_level, 
                                                 int next_x, int next_y) {
    // HACK: Silenciar warnings de variables no usadas por ahora.
    // En el futuro, usar (x, y) para trazar una línea y evitar que el auto
    // atraviese paredes si va muy rápido (tunneling).
    (void)x;
    (void)y;

    CollisionResult result;
    
    // 1. Detectar rampas para saber si cambiamos de nivel automáticamente
    // (Esta lógica se podría mover fuera si prefieres control manual)
    result.is_on_ramp = isRamp(next_x, next_y);
    
    // Lógica básica de cambio de nivel:
    // Si estoy en rampa y en nivel 0, intento ver si puedo subir (si hay puente)
    // Si estoy en rampa y en nivel 1, intento ver si puedo bajar (si hay suelo)
    // Nota: Esto es simplificado. Normalmente se usa la dirección del auto.
    // Por ahora mantenemos el nivel actual a menos que una lógica externa lo cambie.
    result.current_level = current_level;

    // 2. Verificar pared en la posición futura
    result.is_wall = isWall(next_x, next_y, current_level);

    if (result.is_wall) {
        result.can_move = false;
        result.should_bounce = true;
        
        // Calcular la normal para el rebote físico
        calculateSurfaceNormal(next_x, next_y, current_level, 
                               result.normal_x, result.normal_y);
    } else {
        result.can_move = true;
        result.should_bounce = false;
    }

    return result;
}