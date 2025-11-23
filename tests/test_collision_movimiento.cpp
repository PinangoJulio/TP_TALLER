#include <map>
#include <iostream>
#include <SDL2pp/Rect.hh>
#include <SDL2pp/SDL2pp.hh>
#include <SDL2pp/Window.hh>
#include <SDL2/SDL_image.h>
#include <SDL2pp/Renderer.hh>
#include <SDL2pp/Texture.hh>

#include "client_src/game/collision_manager.h"

const int SCREEN_WIDTH = 700;
const int SCREEN_HEIGHT = 700;

// dimensiones visuales del minimapa
const int MINIMAP_SIZE = 200;  
const int MINIMAP_MARGIN = 20; 

const int MINIMAP_SCOPE = 1000; // lo que se muestra

struct Player
{
    float x = 2320.0f; // Posicion inicial de mentiritas
    float y = 2336.0f;
    float speed = 5.0f;
    int dir_x = 0;
    int dir_y = 0;
    int level = 0; // 0 = nivel calle, 1 = nivel puente
};

int getClipIndex(int dx, int dy)
{
    static int last_clip = 4;

    if (dx == 0 && dy == -1)
        last_clip = 0;
    else if (dx == 1 && dy == -1)
        last_clip = 1;
    else if (dx == 1 && dy == 0)
        last_clip = 2;
    else if (dx == 1 && dy == 1)
        last_clip = 3;
    else if (dx == 0 && dy == 1)
        last_clip = 4;
    else if (dx == -1 && dy == 1)
        last_clip = 5;
    else if (dx == -1 && dy == 0)
        last_clip = 6;
    else if (dx == -1 && dy == -1)
        last_clip = 7;

    return last_clip;
}

int main(int argc, char *argv[])
{
    try
    {
        SDL2pp::SDL sdl(SDL_INIT_VIDEO);

        int imgFlags = IMG_INIT_PNG;
        if (!(IMG_Init(imgFlags) & imgFlags))
        {
            throw std::runtime_error("SDL_image no pudo inicializarse: " + std::string(IMG_GetError()));
        }

        SDL2pp::Window window("Test de Colisión - Vice City (Con Rampas)",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        SDL2pp::Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

        // Cargo máscaras de colisión (imágenes blanco nero de la carpeta assets/img/layers)
        std::cout << "Cargando máscaras de colisión..." << std::endl;

        CollisionManager collisionManager(
            "assets/img/map/layers/vice-city/vice-city.png",
            "assets/img/map/layers/vice-city/puentes-transitables.png",
            "assets/img/map/layers/vice-city/rampas.png");

        const int MAP_WIDTH = collisionManager.GetWidth();
        const int MAP_HEIGHT = collisionManager.GetHeight();

        if (MAP_WIDTH == 0 || MAP_HEIGHT == 0)
        {
            throw std::runtime_error("Error: Las máscaras de colisión no se cargaron correctamente");
        }
        std::cout << "Límites del mundo: " << MAP_WIDTH << "x" << MAP_HEIGHT << std::endl;

        // Cargo texturas visuales
        std::cout << "Cargando mapa de Vice City..." << std::endl;
        SDL_Surface *mapSurface = IMG_Load("assets/img/map/cities/vice-city.png");
        if (!mapSurface)
        {
            throw std::runtime_error("Error cargando mapa: " + std::string(IMG_GetError()));
        }
        SDL2pp::Texture mapTexture(renderer, SDL2pp::Surface(mapSurface));

        std::cout << "Cargando sprites de carros..." << std::endl;
        SDL_Surface *carSurface = IMG_Load("assets/img/map/cars/spriteshit-cars.png");
        if (!carSurface)
        {
            carSurface = IMG_Load("spriteshit-cars.png");
            if (!carSurface)
            {
                throw std::runtime_error("Error cargando sprites: " + std::string(IMG_GetError()));
            }
        }

        SDL_SetColorKey(carSurface, SDL_TRUE, SDL_MapRGB(carSurface->format, 0, 0, 0));
        SDL2pp::Texture carTexture(renderer, SDL2pp::Surface(carSurface));

        SDL_Surface *puentesSurf = IMG_Load("assets/img/map/layers/vice-city/vice-city-puentes.png");
        SDL2pp::Texture puentesTexture(renderer, SDL2pp::Surface(puentesSurf ? puentesSurf : SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0)));
        bool hasPuentes = (puentesSurf != nullptr);

        SDL_Surface *topSurface = IMG_Load("assets/img/map/layers/vice-city/vice-city-top.png");
        SDL2pp::Texture topTexture(renderer, SDL2pp::Surface(topSurface ? topSurface : SDL_CreateRGBSurface(0, 1, 1, 32, 0, 0, 0, 0)));
        bool hasTop = (topSurface != nullptr);

        // Clips del sprite sheet
        std::map<int, SDL2pp::Rect> car_clips;
        car_clips[0] = SDL2pp::Rect(32, 0, 32, 32);
        car_clips[1] = SDL2pp::Rect(64, 0, 32, 32);
        car_clips[2] = SDL2pp::Rect(96, 0, 32, 32);
        car_clips[3] = SDL2pp::Rect(128, 0, 32, 32);
        car_clips[4] = SDL2pp::Rect(160, 0, 32, 32);
        car_clips[5] = SDL2pp::Rect(192, 0, 32, 32);
        car_clips[6] = SDL2pp::Rect(224, 0, 32, 32);
        car_clips[7] = SDL2pp::Rect(0, 0, 32, 32);

        Player player;
        bool running = true;
        SDL_Event event;

        std::cout << "\n=== TEST INICIADO (Con Sistema de Rampas) ===" << std::endl;
        std::cout << "Posición: (" << player.x << ", " << player.y << ")" << std::endl;
        std::cout << "Nivel inicial: " << player.level << " (0=calle, 1=puente)" << std::endl;
        std::cout << "\nControles:" << std::endl;
        std::cout << "  Flechas: Mover" << std::endl;
        std::cout << "  ESPACIO: Info de posición" << std::endl;
        std::cout << "  R: Verificar si estás en una rampa" << std::endl;
        std::cout << "  ESC: Salir" << std::endl;

        while (running)
        {

            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_QUIT)
                {
                    running = false;
                }
                if (event.type == SDL_KEYDOWN)
                {
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                    {
                        running = false;
                    }
                    if (event.key.keysym.sym == SDLK_SPACE)
                    {
                        std::cout << "Pos: (" << player.x << ", " << player.y << ") | Nivel: " << player.level;
                        std::cout << " | Camino: " << (collisionManager.hasGroundLevel(player.x, player.y) ? "Sí" : "No");
                        std::cout << " | Puente: " << (collisionManager.hasBridgeLevel(player.x, player.y) ? "Sí" : "No");
                        std::cout << " | Rampa: " << (collisionManager.isRamp(player.x, player.y) ? "Sí" : "No") << std::endl;
                    }
                }
            }

            const Uint8 *state = SDL_GetKeyboardState(NULL);

            // Movimiento 4 direcciones
            player.dir_x = 0;
            player.dir_y = 0;

            if (state[SDL_SCANCODE_UP])
            {
                player.dir_y = -1;
                player.dir_x = 0;
            }
            else if (state[SDL_SCANCODE_DOWN])
            {
                player.dir_y = 1;
                player.dir_x = 0;
            }
            else if (state[SDL_SCANCODE_LEFT])
            {
                player.dir_x = -1;
                player.dir_y = 0;
            }
            else if (state[SDL_SCANCODE_RIGHT])
            {
                player.dir_x = 1;
                player.dir_y = 0;
            }

            // LÓGICA DE MOVIMIENTO CON TRANSICIONES RESTRINGIDAS
            if (player.dir_x != 0 || player.dir_y != 0)
            {
                float nextX = player.x + (player.dir_x * player.speed);
                float nextY = player.y + (player.dir_y * player.speed);

                // Aseuro que la posición esté dentro de las limitacionas del mapa
                if (nextX < 0)
                    nextX = 0;
                if (nextY < 0)
                    nextY = 0;
                if (nextX >= MAP_WIDTH)
                    nextX = MAP_WIDTH - 1;
                if (nextY >= MAP_HEIGHT)
                    nextY = MAP_HEIGHT - 1;

                bool nextHasGround = collisionManager.hasGroundLevel(nextX, nextY);
                bool nextHasBridge = collisionManager.hasBridgeLevel(nextX, nextY);
                bool didTransition = false;

                // Caso 1: Estoy en calle (nivel 0) y me muevo hacia zona con puente
                if (player.level == 0 && nextHasBridge)
                {
                    // Si la siguiente posición tiene SOLO puente (no camino), necesito rampa
                    if (!nextHasGround)
                    {
                        if (collisionManager.canTransition(nextX, nextY, 0, 1))
                        {
                            player.x = nextX;
                            player.y = nextY;
                            player.level = 1;
                            didTransition = true;
                        }
                    }
                    // Si tiene AMBOS (camino Y puente), es una rampa, me puedo mover
                    else if (nextHasGround && nextHasBridge)
                    {
                        // Estoy en la rampa, me muevo libremente
                        if (!collisionManager.isWall(static_cast<int>(nextX), static_cast<int>(nextY), player.level))
                        {
                            player.x = nextX;
                            player.y = nextY;
                            didTransition = true;
                        }
                    }
                }
                // Caso 2: Estoy en puente (nivel 1) y me muevo hacia zona con camino
                else if (player.level == 1 && nextHasGround)
                {
                    // Si la siguiente posición tiene SOLO camino (no puente), necesito rampa
                    if (!nextHasBridge)
                    {
                        if (collisionManager.canTransition(nextX, nextY, 1, 0))
                        {
                            player.x = nextX;
                            player.y = nextY;
                            player.level = 0;
                            didTransition = true;
                        }
                    }
                    // Si tiene AMBOS (puente Y camino), es una rampa, me puedo mover
                    else if (nextHasBridge && nextHasGround)
                    {
                        if (!collisionManager.isWall(static_cast<int>(nextX), static_cast<int>(nextY), player.level))
                        {
                            player.x = nextX;
                            player.y = nextY;
                            didTransition = true;
                        }
                    }
                }

                // LÓGICA 2: Movimiento normal en el mismo nivel
                if (!didTransition)
                {
                    if (!collisionManager.isWall(static_cast<int>(nextX), static_cast<int>(nextY), player.level))
                    {
                        player.x = nextX;
                        player.y = nextY;
                    }
                }
            }

            // RENDER
            renderer.Clear();

            SDL2pp::Rect viewport(
                static_cast<int>(player.x - SCREEN_WIDTH / 2),
                static_cast<int>(player.y - SCREEN_HEIGHT / 2),
                SCREEN_WIDTH,
                SCREEN_HEIGHT);

            if (viewport.x < 0)
                viewport.x = 0;
            if (viewport.y < 0)
                viewport.y = 0;
            if (viewport.x + viewport.w > MAP_WIDTH)
                viewport.x = MAP_WIDTH - viewport.w;
            if (viewport.y + viewport.h > MAP_HEIGHT)
                viewport.y = MAP_HEIGHT - viewport.h;

            SDL2pp::Rect screen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

            // Mapa base
            renderer.Copy(mapTexture, viewport, screen);

            // Puentes (si estamos en nivel 1)
            if (hasPuentes && player.level == 1)
            {
                renderer.Copy(puentesTexture, viewport, screen);
            }

            // Jugador
            int clip_index = getClipIndex(player.dir_x, player.dir_y);
            SDL2pp::Rect car_clip = car_clips[clip_index];

            int screen_x = static_cast<int>(player.x) - viewport.x - car_clip.w / 2;
            int screen_y = static_cast<int>(player.y) - viewport.y - car_clip.h / 2;

            SDL2pp::Rect car_dest(screen_x, screen_y, car_clip.w, car_clip.h);
            renderer.Copy(carTexture, car_clip, car_dest);

            // Puentes (si estamos en nivel 0)
            if (hasPuentes && player.level == 0)
            {
                renderer.Copy(puentesTexture, viewport, screen);
            }

            // Capa TOP
            if (hasTop)
            {
                renderer.Copy(topTexture, viewport, screen);
            }
           
            
            // RENDERIZADO DEL MINIMAPA

            int minimapSrcX = static_cast<int>(player.x) - (MINIMAP_SCOPE / 2);
            int minimapSrcY = static_cast<int>(player.y) - (MINIMAP_SCOPE / 2);

            // defino los bordes del mapa para que la cámara no se salga
            if (minimapSrcX < 0)
                minimapSrcX = 0;
            if (minimapSrcY < 0)
                minimapSrcY = 0;
            if (minimapSrcX + MINIMAP_SCOPE > MAP_WIDTH)
                minimapSrcX = MAP_WIDTH - MINIMAP_SCOPE;
            if (minimapSrcY + MINIMAP_SCOPE > MAP_HEIGHT)
                minimapSrcY = MAP_HEIGHT - MINIMAP_SCOPE;

            SDL2pp::Rect minimapSrc(minimapSrcX, minimapSrcY, MINIMAP_SCOPE, MINIMAP_SCOPE);

            // dónde se dibuja en la pantalla
            SDL2pp::Rect minimapDest(
                SCREEN_WIDTH - MINIMAP_SIZE - MINIMAP_MARGIN,  // Posición X
                SCREEN_HEIGHT - MINIMAP_SIZE - MINIMAP_MARGIN, // Posici+on Y
                MINIMAP_SIZE,                                  // Ancho
                MINIMAP_SIZE                                   // Alto
            );

            // Dibujar fondo del minimapa por si pasa algo con la imagen
            renderer.SetDrawColor(0, 0, 0, 255); 
            renderer.FillRect(minimapDest);

            // Dibujar el mapa original dentro del cuadrado
            renderer.Copy(mapTexture, minimapSrc, minimapDest);

            // Dibujar el borde del minimapa
            renderer.SetDrawColor(255, 255, 255, 255); // Borde blanco
            renderer.DrawRect(minimapDest);

        
            // Factor de escala: (Tamaño Pantalla / Tamaño Mundo Real)
            float scale = (float)MINIMAP_SIZE / (float)MINIMAP_SCOPE;

            // Posición relativa del jugador respecto al recorte
            float playerRelX = player.x - minimapSrcX;
            float playerRelY = player.y - minimapSrcY;

            // Posición final del jugador en pantalla dentro del minimapa
            int dotX = minimapDest.x + (playerRelX * scale);
            int dotY = minimapDest.y + (playerRelY * scale);
            int dotSize = 4; 

            // Dibujar al jugador en el minimapa
            renderer.SetDrawColor(255, 255, 255, 255); 
            int borderSize = 8;                       
            SDL2pp::Rect playerBorder(dotX - borderSize / 2, dotY - borderSize / 2, borderSize, borderSize);
            renderer.FillRect(playerBorder);

            // cOlor jugador 
            renderer.SetDrawColor(0, 255, 255, 255); // Testear otros colores porque azul medio que no se ve mucho
            int innerSize = 6;                       
            SDL2pp::Rect playerInner(dotX - innerSize / 2, dotY - innerSize / 2, innerSize, innerSize);
            renderer.FillRect(playerInner);
           
            renderer.Present();

            SDL_Delay(16);
        }

        std::cout << "\n=== TEST FINALIZADO ===" << std::endl;
        IMG_Quit();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}