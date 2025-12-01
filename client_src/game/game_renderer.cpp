#include "game_renderer.h"
#include <iostream>
#include <cmath>
#include <algorithm> 
#include <cctype>    
#include <SDL2/SDL_image.h>
#include <yaml-cpp/yaml.h>
#include <fstream> 

const float RAD_TO_DEG = 180.0f / M_PI;

GameRenderer::GameRenderer(SDL2pp::Renderer& renderer_ref)
    : renderer(renderer_ref),
      map_width(0),
      map_height(0) {

    // Cargar spritesheet de autos
    car_texture = std::make_unique<SDL2pp::Texture>(renderer, 
        SDL2pp::Surface(IMG_Load("assets/img/map/cars/spritesheet-cars.png")));

    // Inicializar clips
    car_clips[0] = SDL2pp::Rect(32, 0, 32, 32);
    car_clips[1] = SDL2pp::Rect(64, 0, 32, 32);
    car_clips[2] = SDL2pp::Rect(96, 0, 32, 32);
    car_clips[3] = SDL2pp::Rect(128, 0, 32, 32);
    car_clips[4] = SDL2pp::Rect(160, 0, 32, 32);
    car_clips[5] = SDL2pp::Rect(192, 0, 32, 32);
    car_clips[6] = SDL2pp::Rect(224, 0, 32, 32);
    car_clips[7] = SDL2pp::Rect(0, 0, 32, 32);
}

void GameRenderer::init_race(const std::string& yaml_path) {
    std::cout << "[GameRenderer] Inicializando carrera. Config: " << yaml_path << std::endl;

    try {
        YAML::Node config = YAML::LoadFile(yaml_path);

        if (!config["race"] || !config["race"]["city"] || !config["race"]["name"]) {
            throw std::runtime_error("El archivo YAML no tiene los campos 'race.city' o 'race.name'");
        }

        std::string raw_city = config["race"]["city"].as<std::string>(); 
        std::string raw_name = config["race"]["name"].as<std::string>(); 

        // Normalizar nombres (todo minúscula, espacios/guiones bajos a guiones medios)
        auto normalize_path_name = [](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c){ return std::tolower(c); });
            std::replace(s.begin(), s.end(), '_', '-');
            std::replace(s.begin(), s.end(), ' ', '-');
            return s;
        };

        std::string city_name = normalize_path_name(raw_city); 
        std::string route_name = normalize_path_name(raw_name);

        std::cout << "[GameRenderer] Info -> Ciudad: " << city_name << " | Ruta: " << route_name << std::endl;

        // Construir rutas de assets
        std::string visual_base = "assets/img/map/cities/";
        std::string map_file = visual_base + city_name + ".png";

        std::string layer_root = "assets/img/map/layers/" + city_name + "/";// + route_name + "/";
        std::string collision_file = layer_root + "camino.png";
        std::string bridges_mask   = layer_root + "puentes.png";
        std::string ramps_file     = layer_root + "rampas.png";
        std::string top_file       = layer_root + "top.png";
        std::string bridges_visual = layer_root + "puentes-top.png";

        std::string minimap_file = "assets/img/map/cities/caminos/" + city_name + "/" + route_name + "/debug_resultado_v5.png";

        std::cout << "[GameRenderer] Cargando Assets..." << std::endl;
        std::cout << "  - Minimapa: " << minimap_file << std::endl;

        // Mapa Visual
        SDL_Surface* surfMap = IMG_Load(map_file.c_str());
        if (!surfMap) {
            throw std::runtime_error("Fallo al cargar mapa visual: " + map_file);
        }
        map_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfMap));
        map_width = map_texture->GetWidth();
        map_height = map_texture->GetHeight();

        // Puentes Visual
        SDL_Surface* surfPuentes = IMG_Load(bridges_visual.c_str());
        if (surfPuentes) {
            puentes_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfPuentes));
        } else {
            puentes_texture.reset();
        }

        // Top Visual
        SDL_Surface* surfTop = IMG_Load(top_file.c_str());
        if (surfTop) {
            top_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfTop));
        } else {
            top_texture.reset();
        }

        // Minimapa 
        SDL_Surface* surfMini = IMG_Load(minimap_file.c_str());
        if (surfMini) {
            minimap_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfMini));
            std::cout << "[GameRenderer] Minimapa cargado correctamente. Dim: " 
                      << surfMini->w << "x" << surfMini->h << std::endl;
        } else {
            std::cerr << "[GameRenderer] ⚠️  No se pudo cargar el minimapa: " << minimap_file << std::endl;
            minimap_texture.reset();
        }

        // Colisiones
        std::ifstream f(collision_file);
        if (f.good()) {
             collision_manager = std::make_unique<CollisionManager>(collision_file, bridges_mask, ramps_file);
        } else {
             std::cerr << "[GameRenderer] ⚠️  No se encontró collision mask: " << collision_file << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[GameRenderer] ERROR FATAL: " << e.what() << std::endl;
    }
}

int GameRenderer::getClipIndexFromAngle(float angle_radians) {
    float degrees = angle_radians * RAD_TO_DEG;
    while (degrees < 0) degrees += 360;
    while (degrees >= 360) degrees -= 360;
    int index = static_cast<int>((degrees + 22.5f) / 45.0f); 
    return (index + 2) % 8;
}

void GameRenderer::render(const GameState& state, int player_id) {
    if (!map_texture) {
        renderer.SetDrawColor(0, 0, 0, 255);
        renderer.Clear();
        renderer.Present();
        return;
    }

    //  Identificar jugador local
    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == player_id) {
            local_player = &p;
            break;
        }
    }

    // // Debug: Imprimir estado cada cierto tiempo si no se encuentra el jugador o el minimapa
    // static int debug_timer = 0;
    // debug_timer++;
    // if (debug_timer > 120) { // Cada ~2 segundos a 60fps
    //     if (!local_player) {
    //         std::cout << "[GameRenderer Debug] ⚠️ Local player NOT FOUND. ID buscado: " << player_id 
    //                   << ". Jugadores en estado: " << state.players.size() << std::endl;
    //         for(const auto& p : state.players) std::cout << " - PID: " << p.player_id << std::endl;
    //     } else if (!minimap_texture) {
    //         std::cout << "[GameRenderer Debug] ⚠️ Minimap texture is NULL" << std::endl;
    //     }
    //     debug_timer = 0;
    // }

    //Definir foco de la cámara 
    float focus_x = 0;
    float focus_y = 0;

    if (local_player) {
        focus_x = local_player->pos_x;
        focus_y = local_player->pos_y;
    } else {
        // Si no hay jugador, centramos en el medio del mapa para ver algo, o en (0,0)
        
        focus_x = 0;
        focus_y = 0;
    }

    // 3. Calcular Viewport de Cámara Principal
    int cam_x = static_cast<int>(focus_x) - SCREEN_WIDTH / 2;
    int cam_y = static_cast<int>(focus_y) - SCREEN_HEIGHT / 2;

    // Clamping cámara 
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > map_width - SCREEN_WIDTH) cam_x = map_width - SCREEN_WIDTH;
    if (cam_y > map_height - SCREEN_HEIGHT) cam_y = map_height - SCREEN_HEIGHT;

    SDL2pp::Rect viewport(cam_x, cam_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL2pp::Rect screen_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();

    // --- RENDERIZADO PRINCIPAL ---
    
    // Mapa Base
    renderer.Copy(*map_texture, viewport, screen_rect);

    //Jugadores
    for (const auto& player : state.players) {
        int screen_x = static_cast<int>(player.pos_x) - cam_x;
        int screen_y = static_cast<int>(player.pos_y) - cam_y;

        int clip_idx = getClipIndexFromAngle(player.angle);
        if (car_clips.find(clip_idx) == car_clips.end()) clip_idx = 0;
        
        SDL2pp::Rect clip = car_clips[clip_idx];
        SDL2pp::Rect dest(screen_x - clip.w / 2, screen_y - clip.h / 2, clip.w, clip.h);

        if (player.is_alive) {
            renderer.Copy(*car_texture, clip, dest);
        }
    }

    //  Capas superiores (Puentes y Top)
    if (puentes_texture) renderer.Copy(*puentes_texture, viewport, screen_rect);
    if (top_texture) renderer.Copy(*top_texture, viewport, screen_rect);


    // RENDERIZADO DEL MINIMAPA
    if (minimap_texture) { // Renderizar minimapa aunque no haya jugador (mostrará la zona de la cámara)
        
       
        int minimapSrcX = static_cast<int>(focus_x) - (MINIMAP_SCOPE / 2);
        int minimapSrcY = static_cast<int>(focus_y) - (MINIMAP_SCOPE / 2);

        // Clamp del viewport del minimapa para no salir de la textura
        if (minimapSrcX < 0) minimapSrcX = 0;
        if (minimapSrcY < 0) minimapSrcY = 0;
        if (minimapSrcX + MINIMAP_SCOPE > map_width) minimapSrcX = map_width - MINIMAP_SCOPE;
        if (minimapSrcY + MINIMAP_SCOPE > map_height) minimapSrcY = map_height - MINIMAP_SCOPE;

        SDL2pp::Rect minimapSrc(minimapSrcX, minimapSrcY, MINIMAP_SCOPE, MINIMAP_SCOPE);

        // Posición en pantalla 
        SDL2pp::Rect minimapDest(
            SCREEN_WIDTH - MINIMAP_SIZE - MINIMAP_MARGIN,
            SCREEN_HEIGHT - MINIMAP_SIZE - MINIMAP_MARGIN,
            MINIMAP_SIZE,
            MINIMAP_SIZE
        );

        // Fondo negro (marco de seguridad)
        renderer.SetDrawColor(0, 0, 0, 255);
        renderer.FillRect(minimapDest);

        // Dibujar textura del minimapa (camino pintado)
        renderer.Copy(*minimap_texture, minimapSrc, minimapDest);

        // Borde blanco del minimapa
        renderer.SetDrawColor(255, 255, 255, 255);
        renderer.DrawRect(minimapDest);

        // Dibujar al jugador - SOLO SI EXISTE
        if (local_player) {
            // Factor de escala
            float scale = (float)MINIMAP_SIZE / (float)MINIMAP_SCOPE;

            // Posición relativa del jugador respecto al recorte
            float playerRelX = local_player->pos_x - minimapSrcX;
            float playerRelY = local_player->pos_y - minimapSrcY;

            // Posición final en pantalla
            int dotX = minimapDest.x + (playerRelX * scale);
            int dotY = minimapDest.y + (playerRelY * scale);

            // Borde del punto 
            renderer.SetDrawColor(255, 255, 255, 255); 
            int borderSize = 8;
            SDL2pp::Rect playerBorder(dotX - borderSize / 2, dotY - borderSize / 2, borderSize, borderSize);
            renderer.FillRect(playerBorder);

            // Interior del punto 
            renderer.SetDrawColor(0, 255, 255, 255); 
            int innerSize = 6;                       
            SDL2pp::Rect playerInner(dotX - innerSize / 2, dotY - innerSize / 2, innerSize, innerSize);
            renderer.FillRect(playerInner);
        }
    }

    renderer.Present();
}