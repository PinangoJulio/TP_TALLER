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
    : renderer(renderer_ref), map_width(0), map_height(0) {

    // Cargar 3 spritesheets separados por tamaño
    car_texture_32 = std::make_unique<SDL2pp::Texture>(renderer, 
        SDL2pp::Surface(IMG_Load("assets/img/map/cars/spritesheet-cars-32.png")));
    
    car_texture_40 = std::make_unique<SDL2pp::Texture>(renderer, 
        SDL2pp::Surface(IMG_Load("assets/img/map/cars/spritesheet-cars-40.png")));
    
    car_texture_50 = std::make_unique<SDL2pp::Texture>(renderer, 
        SDL2pp::Surface(IMG_Load("assets/img/map/cars/spritesheet-cars-50.png")));

    // Mapper de nombres a info del auto
    struct CarInfo {
        int texture_id;  // 0=32px, 1=40px, 2=50px
        int row;         // Fila en su spritesheet correspondiente
        int size;        // Tamaño del sprite
    };

    car_info_map["J-Classic 600"]   = {0, 0, 32};  // spritesheet-32, fila 0
    car_info_map["Stallion GT"]     = {1, 0, 40};  // spritesheet-40, fila 0
    car_info_map["Cavallo V8"]      = {1, 1, 40};  // spritesheet-40, fila 1
    car_info_map["Leyenda Urbana"]  = {1, 2, 40};  // spritesheet-40, fila 2
    car_info_map["Brisa"]           = {1, 3, 40};  // spritesheet-40, fila 3
    car_info_map["Nómada"]          = {1, 4, 40};  // spritesheet-40, fila 4
    car_info_map["Senator"]         = {2, 0, 50};  // spritesheet-50, fila 0

    // Generar clips para cada spritesheet (8 orientaciones por fila)
    // Spritesheet 32x32
    for (int row = 0; row < 1; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_32[row][dir] = SDL2pp::Rect(dir * 32, row * 32, 32, 32);
        }
    }
    
    // Spritesheet 40x40
    for (int row = 0; row < 5; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_40[row][dir] = SDL2pp::Rect(dir * 40, row * 40, 40, 40);
        }
    }
    
    // Spritesheet 50x50
    for (int row = 0; row < 1; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_50[row][dir] = SDL2pp::Rect(dir * 50, row * 50, 50, 50);
        }
    }
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

        std::string layer_root = "assets/img/map/layers/" + city_name + "/";
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
    
    // 8 direcciones: cada 45 grados
    int index = static_cast<int>((degrees + 22.5f) / 45.0f) % 8;
    
    return index;
}

void GameRenderer::render(const GameState& state, int player_id) {
    if (!map_texture) {
        renderer.SetDrawColor(0, 0, 0, 255);
        renderer.Clear();
        renderer.Present();
        return;
    }

    // Identificar jugador local
    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == player_id) {
            local_player = &p;
            break;
        }
    }

    // Definir foco de la cámara 
    float focus_x = local_player ? local_player->pos_x : 0;
    float focus_y = local_player ? local_player->pos_y : 0;

    // Calcular Viewport de Cámara Principal
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

    // Mapa Base
    renderer.Copy(*map_texture, viewport, screen_rect);

    // Jugadores
    for (const auto& player : state.players) {
        if (!player.is_alive) continue;

        int screen_x = static_cast<int>(player.pos_x) - cam_x;
        int screen_y = static_cast<int>(player.pos_y) - cam_y;

        // Obtener info del auto
        auto it = car_info_map.find(player.car_name);
        if (it == car_info_map.end()) {
            std::cerr << "[GameRenderer] ⚠️ Auto desconocido: " << player.car_name << std::endl;
            continue;
        }

        int texture_id = it->second.texture_id;
        int row = it->second.row;
        int clip_idx = getClipIndexFromAngle(player.angle);

        // Seleccionar textura y clip según el tamaño
        SDL2pp::Texture* texture = nullptr;
        SDL2pp::Rect clip;

        if (texture_id == 0) {
            texture = car_texture_32.get();
            clip = car_clips_32[row][clip_idx];
        } else if (texture_id == 1) {
            texture = car_texture_40.get();
            clip = car_clips_40[row][clip_idx];
        } else if (texture_id == 2) {
            texture = car_texture_50.get();
            clip = car_clips_50[row][clip_idx];
        }

        if (texture) {
            SDL2pp::Rect dest(screen_x - clip.w / 2, screen_y - clip.h / 2, clip.w, clip.h);
            renderer.Copy(*texture, clip, dest);
        }
    }

    // Capas superiores
    if (puentes_texture) renderer.Copy(*puentes_texture, viewport, screen_rect);
    if (top_texture) renderer.Copy(*top_texture, viewport, screen_rect);

    // Minimapa
    if (minimap_texture && local_player) {
        int minimapSrcX = static_cast<int>(focus_x) - (MINIMAP_SCOPE / 2);
        int minimapSrcY = static_cast<int>(focus_y) - (MINIMAP_SCOPE / 2);

        if (minimapSrcX < 0) minimapSrcX = 0;
        if (minimapSrcY < 0) minimapSrcY = 0;
        if (minimapSrcX + MINIMAP_SCOPE > map_width) minimapSrcX = map_width - MINIMAP_SCOPE;
        if (minimapSrcY + MINIMAP_SCOPE > map_height) minimapSrcY = map_height - MINIMAP_SCOPE;

        SDL2pp::Rect minimapSrc(minimapSrcX, minimapSrcY, MINIMAP_SCOPE, MINIMAP_SCOPE);
        SDL2pp::Rect minimapDest(
            SCREEN_WIDTH - MINIMAP_SIZE - MINIMAP_MARGIN,
            SCREEN_HEIGHT - MINIMAP_SIZE - MINIMAP_MARGIN,
            MINIMAP_SIZE,
            MINIMAP_SIZE
        );

        renderer.SetDrawColor(0, 0, 0, 255);
        renderer.FillRect(minimapDest);
        renderer.Copy(*minimap_texture, minimapSrc, minimapDest);
        renderer.SetDrawColor(255, 255, 255, 255);
        renderer.DrawRect(minimapDest);

        float scale = (float)MINIMAP_SIZE / (float)MINIMAP_SCOPE;
        float playerRelX = local_player->pos_x - minimapSrcX;
        float playerRelY = local_player->pos_y - minimapSrcY;
        int dotX = minimapDest.x + (playerRelX * scale);
        int dotY = minimapDest.y + (playerRelY * scale);

        renderer.SetDrawColor(255, 255, 255, 255);
        SDL2pp::Rect playerBorder(dotX - 4, dotY - 4, 8, 8);
        renderer.FillRect(playerBorder);
        renderer.SetDrawColor(0, 255, 255, 255);
        SDL2pp::Rect playerInner(dotX - 3, dotY - 3, 6, 6);
        renderer.FillRect(playerInner);
    }

    renderer.Present();
}