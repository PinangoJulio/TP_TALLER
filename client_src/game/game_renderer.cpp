#include "game_renderer.h"
#include <iostream>
#include <cmath>
#include <SDL2/SDL_image.h>
#include <yaml-cpp/yaml.h>
#include <fstream> // Necesario para verificar existencia de archivos

const float RAD_TO_DEG = 180.0f / M_PI;

GameRenderer::GameRenderer(SDL2pp::Renderer& renderer_ref)
    : renderer(renderer_ref),
      map_width(0),  // Inicializamos en 0 porque aún no cargamos la imagen del mapa
      map_height(0) {

    
    // Si falla, SDL2pp lanzará una excepción
    car_texture = std::make_unique<SDL2pp::Texture>(renderer, 
        SDL2pp::Surface(IMG_Load("assets/img/map/cars/spriteshit-cars.png")));

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
    std::cout << "[GameRenderer] Cargando carrera desde: " << yaml_path << std::endl;

    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        
        // Obtener el nombre de la ciudad del YAML
        std::string city = "Vice City"; // Default
        if (config["race"] && config["race"]["city"]) {
            city = config["race"]["city"].as<std::string>();
        }

        std::cout << "[GameRenderer] Ciudad detectada: " << city << std::endl;

        // Definir rutas base
        std::string visual_base = "assets/img/map/cities/";
        std::string layer_base = "assets/img/map/layers/";
        
        std::string map_file, bridges_file, top_file;
        std::string collision_file, bridges_mask, ramps_file;

        // Selección de archivos según la ciudad
        if (city == "Vice_City" || city == "Vice City") {
            map_file = visual_base + "vice-city.png";
            bridges_file = layer_base + "vice-city/vice-city-puentes.png";
            top_file = layer_base + "vice-city/vice-city-top.png";
            
            collision_file = layer_base + "vice-city/vice-city.png";
            bridges_mask = layer_base + "vice-city/puentes-transitables.png";
            ramps_file = layer_base + "vice-city/rampas.png";
        } 
        else if (city == "Liberty_City" || city == "Liberty City") {
            map_file = visual_base + "liberty-city.png";
            // Configura aquí los archivos para Liberty City cuando los tengas
        } 
        else {
            // San Andreas o Default
            map_file = visual_base + "san-andreas.png";
        }

        // Cargar texturas visuales
        SDL_Surface* surfMap = IMG_Load(map_file.c_str());
        if (!surfMap) throw std::runtime_error("Fallo al cargar mapa: " + std::string(IMG_GetError()));
        
        map_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfMap));
        
        // AHORA SÍ sabemos cuánto mide el mapa real
        map_width = map_texture->GetWidth();
        map_height = map_texture->GetHeight();

        // Cargar capas opcionales (Puentes)
        SDL_Surface* surfPuentes = IMG_Load(bridges_file.c_str());
        if (surfPuentes) {
            puentes_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfPuentes));
        } else {
            puentes_texture.reset(); // Liberar si existía de una carrera anterior
        }

        // Cargar capas opcionales (Techos)
        SDL_Surface* surfTop = IMG_Load(top_file.c_str());
        if (surfTop) {
            top_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfTop));
        } else {
            top_texture.reset();
        }

        // Inicializar CollisionManager (solo si existen los archivos)
        std::ifstream f(collision_file);
        if (f.good()) {
             collision_manager = std::make_unique<CollisionManager>(collision_file, bridges_mask, ramps_file);
        }

        std::cout << "[GameRenderer] Carrera cargada exitosamente. Dimensiones del mapa: " 
                  << map_width << "x" << map_height << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[GameRenderer] ERROR FATAL cargando carrera: " << e.what() << std::endl;
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
        // Pantalla negra si no hay mapa cargado
        renderer.SetDrawColor(0, 0, 0, 255);
        renderer.Clear();
        renderer.Present();
        return;
    }

    // 1. Calcular Cámara (Centrada en el jugador local)
    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == player_id) {
            local_player = &p;
            break;
        }
    }

    int cam_x = 0, cam_y = 0;
    
    if (local_player) {
        // Centrar: Posición - Mitad de Pantalla
        cam_x = static_cast<int>(local_player->pos_x) - SCREEN_WIDTH / 2;
        cam_y = static_cast<int>(local_player->pos_y) - SCREEN_HEIGHT / 2;
    }

    // Clamping (Evitar que la cámara se salga de los bordes del MAPA)
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > map_width - SCREEN_WIDTH) cam_x = map_width - SCREEN_WIDTH;
    if (cam_y > map_height - SCREEN_HEIGHT) cam_y = map_height - SCREEN_HEIGHT;

    SDL2pp::Rect viewport(cam_x, cam_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL2pp::Rect screen_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();

    // --- CAPA 1: MAPA BASE ---
    renderer.Copy(*map_texture, viewport, screen_rect);

    // --- CAPA 2: JUGADORES ---
    for (const auto& player : state.players) {
        int screen_x = static_cast<int>(player.pos_x) - cam_x;
        int screen_y = static_cast<int>(player.pos_y) - cam_y;

        int clip_idx = getClipIndexFromAngle(player.angle);
        if (car_clips.find(clip_idx) == car_clips.end()) clip_idx = 0;
        
        SDL2pp::Rect clip = car_clips[clip_idx];
        SDL2pp::Rect dest(screen_x - clip.w / 2, screen_y - clip.h / 2, clip.w, clip.h);

        // Solo dibujar si está vivo (o dibujar explosión si no)
        if (player.is_alive) {
            renderer.Copy(*car_texture, clip, dest);
        }
    }

    // --- CAPA 3: PUENTES (Para dar efecto de profundidad si pasas por abajo) ---
    if (puentes_texture) {
       renderer.Copy(*puentes_texture, viewport, screen_rect);
    }

    // --- CAPA 4: TOP (Techos de edificios, árboles altos) ---
    if (top_texture) {
        renderer.Copy(*top_texture, viewport, screen_rect);
    }

    renderer.Present();
}