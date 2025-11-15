#include <map>
#include <iostream>
#include <SDL2pp/Rect.hh>
#include <SDL2pp/SDL2pp.hh>
#include <SDL2pp/Window.hh>
#include <SDL2/SDL_image.h>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

#include "client_src/game/collision_manager.h"

// Tamaño de nuestra ventana
const int SCREEN_WIDTH = 700;
const int SCREEN_HEIGHT = 700;

// Struct simple para el jugador
struct Player {
    float x = 2320.0f;
    float y = 2336.0f;
    float speed = 5.0f;
    int dir_x = 0;
    int dir_y = -1;
    int level = 0;  // 0 = nivel calle, 1 = nivel puente
};

// Mapea la dirección (dx, dy) a un índice de clip (0-7)
int getClipIndex(int dx, int dy) {
    if (dx == 0 && dy == -1) return 0; // Arriba
    if (dx == 1 && dy == -1) return 1; // Arriba-Derecha
    if (dx == 1 && dy == 0) return 2;  // Derecha
    if (dx == 1 && dy == 1) return 3;  // Abajo-Derecha
    if (dx == 0 && dy == 1) return 4;  // Abajo
    if (dx == -1 && dy == 1) return 5; // Abajo-Izquierda
    if (dx == -1 && dy == 0) return 6;  // Izquierda
    if (dx == -1 && dy == -1) return 7; // Arriba-Izquierda
    return 4; // Default: Abajo
}

int main(int argc, char* argv[]) {
    try {
        // Inicializar SDL y SDL_image
        SDL2pp::SDL sdl(SDL_INIT_VIDEO);
        
        // Inicializar SDL_image manualmente
        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags)) {
            throw std::runtime_error("SDL_image no pudo inicializarse: " + std::string(IMG_GetError()));
        }

        // Crear ventana y renderer
        SDL2pp::Window window("Test de Colisión - Vice City (Sistema de Niveles)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        SDL2pp::Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

        // --- Cargar Máscaras de Colisión ---
        std::cout << "Cargando máscaras de colisión..." << std::endl;
        CollisionManager collisionManager(
            "assets/img/layers/camino.png",
            "assets/img/layers/puentes.png"
            //"assets/img/layers/top.png"
        );

        // --- Cargar Mapa Visible ---
        std::cout << "Cargando mapa de Vice City..." << std::endl;
        SDL_Surface* mapSurf = IMG_Load("assets/img/map/cities/vice-city.png");
        if (!mapSurf) {
            throw std::runtime_error("Error cargando mapa: " + std::string(IMG_GetError()));
        }
        std::cout << "Mapa cargado: " << mapSurf->w << "x" << mapSurf->h << std::endl;
        SDL2pp::Texture mapTexture(renderer, SDL2pp::Surface(mapSurf));

        // --- Cargar Sprites de Carros ---
        std::cout << "Cargando sprites de carros..." << std::endl;
        SDL_Surface* carSurf = IMG_Load("assets/img/map/cars/spriteshit.png");
        if (!carSurf) {
            throw std::runtime_error("Error cargando sprites: " + std::string(IMG_GetError()));
        }
        SDL_SetColorKey(carSurf, SDL_TRUE, SDL_MapRGB(carSurf->format, 0, 0, 0)); 
        SDL2pp::Texture carTexture(renderer, SDL2pp::Surface(carSurf));

        // --- Cargar Capa de Puentes ---
        SDL_Surface* puentesSurf = IMG_Load("assets/img/layers/vice-city-puentes.png");
        SDL2pp::Texture puentesTexture(renderer, SDL2pp::Surface(puentesSurf ? puentesSurf : SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0)));
        bool hasPuentes = (puentesSurf != nullptr);

        // --- Cargar Capa Top ---
        SDL_Surface* topSurf = IMG_Load("assets/img/layers/vice-city-top.png");
        SDL2pp::Texture topTexture(renderer, SDL2pp::Surface(topSurf ? topSurf : SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0)));
        bool hasTop = (topSurf != nullptr);

        // --- Clips del Sprite Sheet ---
        std::map<int, SDL2pp::Rect> car_clips;
        car_clips[0] = SDL2pp::Rect(0, 0, 32, 32);
        car_clips[1] = SDL2pp::Rect(32, 0, 32, 32);
        car_clips[2] = SDL2pp::Rect(64, 0, 32, 32);
        car_clips[3] = SDL2pp::Rect(96, 0, 32, 32);
        car_clips[4] = SDL2pp::Rect(128, 0, 32, 32);
        car_clips[5] = SDL2pp::Rect(160, 0, 32, 32);
        car_clips[6] = SDL2pp::Rect(192, 0, 32, 32);
        car_clips[7] = SDL2pp::Rect(224, 0, 32, 32);

        Player player;
        bool running = true;
        SDL_Event event;

        std::cout << "\n=== TEST INICIADO (Sistema de Niveles) ===" << std::endl;
        std::cout << "Posición: (" << player.x << ", " << player.y << ")" << std::endl;
        std::cout << "Nivel inicial: " << player.level << " (0=calle, 1=puente)" << std::endl;
        std::cout << "\nControles:" << std::endl;
        std::cout << "  Flechas: Mover" << std::endl;
        std::cout << "  ESPACIO: Info de posición" << std::endl;
        std::cout << "  L: Cambiar nivel manualmente (para debug)" << std::endl;
        std::cout << "  ESC: Salir" << std::endl;

        // --- Game Loop temporal ---
        while (running) {
            // --- Input ---
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN) {
                    if (event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    if (event.key.keysym.sym == SDLK_SPACE) {
                        std::cout << "Pos: (" << player.x << ", " << player.y << ") | Nivel: " << player.level;
                        std::cout << " | Camino: " << (collisionManager.hasGroundLevel(player.x, player.y) ? "Sí" : "No");
                        std::cout << " | Puente: " << (collisionManager.hasBridgeLevel(player.x, player.y) ? "Sí" : "No") << std::endl;
                    }
                    
                    if (event.key.keysym.sym == SDLK_l) {
                        player.level = 1 - player.level;
                        std::cout << "Nivel cambiado a: " << player.level << std::endl;
                    }
                }
            }

            const Uint8* state = SDL_GetKeyboardState(NULL);
            int dx = 0;
            int dy = 0;

            if (state[SDL_SCANCODE_UP]) dy = -1;
            if (state[SDL_SCANCODE_DOWN]) dy = 1;
            if (state[SDL_SCANCODE_LEFT]) dx = -1;
            if (state[SDL_SCANCODE_RIGHT]) dx = 1;

            if (dx != 0 || dy != 0) {
                player.dir_x = dx;
                player.dir_y = dy;
            }
            
            // --- Update ---
            float nextX = player.x + (player.dir_x * player.speed);
            float nextY = player.y + (player.dir_y * player.speed);

            // Limitar al borde del mapa
            if (nextX < 0) nextX = 0;
            if (nextY < 0) nextY = 0;
            if (nextX >= 4640) nextX = 4639;
            if (nextY >= 4672) nextY = 4671;

            // Detección de colisión según el nivel actual
            if (!collisionManager.isWall(static_cast<int>(nextX), static_cast<int>(nextY), player.level)) {
                player.x = nextX;
                player.y = nextY;
                
                // TRANSICIÓN AUTOMÁTICA DE NIVELES
                // Si estoy en nivel 0 y llego a un puente (zona de rampa), subo
                // Si estoy en nivel 1 y no hay más puente, bajo
                bool hasGround = collisionManager.hasGroundLevel(player.x, player.y);
                bool hasBridge = collisionManager.hasBridgeLevel(player.x, player.y);
                
                if (player.level == 0 && hasBridge && hasGround) {
                    // Zona de transición: ambos niveles disponibles
                    // Mantenemos nivel actual por ahora
                } else if (player.level == 0 && hasBridge && !hasGround) {
                    // Solo hay puente, forzar subir
                    player.level = 1;
                    std::cout << "↑ Subiendo a puente" << std::endl;
                } else if (player.level == 1 && hasGround && !hasBridge) {
                    // Solo hay calle, forzar bajar
                    player.level = 0;
                    std::cout << "↓ Bajando a calle" << std::endl;
                }
            }

            // --- Render ---
            renderer.Clear();

            SDL2pp::Rect viewport(
                static_cast<int>(player.x - SCREEN_WIDTH / 2),
                static_cast<int>(player.y - SCREEN_HEIGHT / 2),
                SCREEN_WIDTH,
                SCREEN_HEIGHT
            );
            SDL2pp::Rect screen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            // Mapa base
            renderer.Copy(mapTexture, viewport, screen);

            // Si estoy en nivel 0 (calle), los puentes me tapan
            if (hasPuentes && player.level == 0) {
                renderer.Copy(puentesTexture, viewport, screen);
            }

            // Jugador
            int clip_index = getClipIndex(player.dir_x, player.dir_y);
            SDL2pp::Rect car_clip = car_clips[clip_index];
            SDL2pp::Rect car_dest(
                SCREEN_WIDTH / 2 - car_clip.w / 2,
                SCREEN_HEIGHT / 2 - car_clip.h / 2,
                car_clip.w,
                car_clip.h
            );
            renderer.Copy(carTexture, car_clip, car_dest);

            // Si estoy en nivel 1 (puente), no me tapan (ya estoy arriba)


            // Top siempre encima (no funca)
            if (hasTop) {
                renderer.Copy(topTexture, viewport, screen);
            }

            renderer.Present();
            SDL_Delay(16);
        }

        std::cout << "\n=== TEST FINALIZADO ===" << std::endl;
        IMG_Quit();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}