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
    car_info_map["J-Classic 600"]   = {0, 0, 32};
    car_info_map["Stallion GT"]     = {1, 0, 40};
    car_info_map["Cavallo V8"]      = {1, 1, 40};
    car_info_map["Leyenda Urbana"]  = {1, 2, 40};
    car_info_map["Brisa"]           = {1, 3, 40};
    car_info_map["Nómada"]          = {1, 4, 40};
    car_info_map["Senator"]         = {2, 0, 50};

    // Generar clips para cada spritesheet (8 orientaciones por fila)
    for (int row = 0; row < 1; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_32[row][dir] = SDL2pp::Rect(dir * 32, row * 32, 32, 32);
        }
    }
    
    for (int row = 0; row < 5; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_40[row][dir] = SDL2pp::Rect(dir * 40, row * 40, 40, 40);
        }
    }
    
    for (int row = 0; row < 1; ++row) {
        for (int dir = 0; dir < 8; ++dir) {
            car_clips_50[row][dir] = SDL2pp::Rect(dir * 50, row * 50, 50, 50);
        }
    }
}

void GameRenderer::init_race(const std::string& yaml_path) {
    std::cout << "[GameRenderer] ═══════════════════════════════════════" << std::endl;
    std::cout << "[GameRenderer] Inicializando carrera. Config: " << yaml_path << std::endl;

    try {
        YAML::Node config = YAML::LoadFile(yaml_path);

        if (!config["race"] || !config["race"]["city"] || !config["race"]["name"]) {
            throw std::runtime_error("El archivo YAML no tiene los campos 'race.city' o 'race.name'");
        }

        std::string raw_city = config["race"]["city"].as<std::string>(); 
        std::string raw_name = config["race"]["name"].as<std::string>(); 

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

        SDL_Surface* surfMap = IMG_Load(map_file.c_str());
        if (!surfMap) {
            throw std::runtime_error("Fallo al cargar mapa visual: " + map_file);
        }
        map_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfMap));
        map_width = map_texture->GetWidth();
        map_height = map_texture->GetHeight();

        SDL_Surface* surfPuentes = IMG_Load(bridges_visual.c_str());
        if (surfPuentes) {
            puentes_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfPuentes));
        } else {
            puentes_texture.reset();
        }

        SDL_Surface* surfTop = IMG_Load(top_file.c_str());
        if (surfTop) {
            top_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfTop));
        } else {
            top_texture.reset();
        }

        SDL_Surface* surfMini = IMG_Load(minimap_file.c_str());
        if (surfMini) {
            minimap_texture = std::make_unique<SDL2pp::Texture>(renderer, SDL2pp::Surface(surfMini));
            std::cout << "[GameRenderer] Minimapa cargado correctamente." << std::endl;
        } else {
            std::cerr << "[GameRenderer] ⚠️  No se pudo cargar el minimapa: " << minimap_file << std::endl;
            minimap_texture.reset();
        }

        std::ifstream f(collision_file);
        if (f.good()) {
             collision_manager = std::make_unique<CollisionManager>(collision_file, bridges_mask, ramps_file);
        } else {
             std::cerr << "[GameRenderer] ⚠️  No se encontró collision mask: " << collision_file << std::endl;
        }

        // Cargar checkpoints
        load_checkpoints_from_yaml(yaml_path);

        std::cout << "[GameRenderer] ✅ Inicialización completada" << std::endl;
        std::cout << "[GameRenderer] ═══════════════════════════════════════" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[GameRenderer] ERROR FATAL: " << e.what() << std::endl;
    }
}

int GameRenderer::getClipIndexFromAngle(float angle_radians) {
    float degrees = angle_radians * RAD_TO_DEG;
    while (degrees < 0) degrees += 360;
    while (degrees >= 360) degrees -= 360;
    
    // 8 direcciones
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

    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == player_id) {
            local_player = &p;
            break;
        }
    }

    float focus_x = local_player ? local_player->pos_x : 0;
    float focus_y = local_player ? local_player->pos_y : 0;

    int cam_x = static_cast<int>(focus_x) - SCREEN_WIDTH / 2;
    int cam_y = static_cast<int>(focus_y) - SCREEN_HEIGHT / 2;

    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > map_width - SCREEN_WIDTH) cam_x = map_width - SCREEN_WIDTH;
    if (cam_y > map_height - SCREEN_HEIGHT) cam_y = map_height - SCREEN_HEIGHT;

    SDL2pp::Rect viewport(cam_x, cam_y, SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL2pp::Rect screen_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();

    renderer.Copy(*map_texture, viewport, screen_rect);

    // Renderizar checkpoints
    render_checkpoints(viewport, cam_x, cam_y);

    for (const auto& player : state.players) {
        if (!player.is_alive) continue;

        int screen_x = static_cast<int>(player.pos_x) - cam_x;
        int screen_y = static_cast<int>(player.pos_y) - cam_y;

        auto it = car_info_map.find(player.car_name);
        if (it == car_info_map.end()) {
            std::cerr << "[GameRenderer] ⚠️ Auto desconocido: " << player.car_name << std::endl;
            continue;
        }

        int texture_id = it->second.texture_id;
        int row = it->second.row;
        int clip_idx = getClipIndexFromAngle(player.angle);

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

    if (puentes_texture) renderer.Copy(*puentes_texture, viewport, screen_rect);
    if (top_texture) renderer.Copy(*top_texture, viewport, screen_rect);

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

void GameRenderer::load_checkpoints_from_yaml(const std::string& yaml_path) {
    std::cout << "[GameRenderer] Cargando checkpoints desde: " << yaml_path << std::endl;
    
    checkpoints.clear();
    spawn_points.clear();
    
    try {
        YAML::Node config = YAML::LoadFile(yaml_path);
        
        if (config["checkpoints"] && config["checkpoints"].IsSequence()) {
            for (const auto& cp : config["checkpoints"]) {
                Checkpoint checkpoint;
                checkpoint.id = cp["id"].as<int>();
                checkpoint.type = cp["type"].as<std::string>();
                checkpoint.x = cp["x"].as<float>();
                checkpoint.y = cp["y"].as<float>();
                checkpoint.width = cp["width"].as<float>();
                checkpoint.height = cp["height"].as<float>();
                checkpoint.angle = cp["angle"] ? cp["angle"].as<float>() : 0.0f;
                
                checkpoints.push_back(checkpoint);
            }
            std::cout << "[GameRenderer]  Cargados " << checkpoints.size() << " checkpoints" << std::endl;
        }
        
        if (config["spawn_points"] && config["spawn_points"].IsSequence()) {
            // FIX: Eliminada variable 'spawn_num' no usada
            for (const auto& sp : config["spawn_points"]) {
                SpawnPoint spawn;
                spawn.x = sp["x"].as<float>();
                spawn.y = sp["y"].as<float>();
                spawn.angle = sp["angle"].as<float>();
                spawn_points.push_back(spawn);
            }
            std::cout << "[GameRenderer] Cargados " << spawn_points.size() << " spawn points" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[GameRenderer]  Error cargando checkpoints: " << e.what() << std::endl;
    }
}

void GameRenderer::render_checkpoints(const SDL2pp::Rect& viewport, int cam_x, int cam_y) {
    (void)viewport; // FIX: Silenciar warning de parámetro no usado

    for (const auto& cp : checkpoints) {
        int screen_x = static_cast<int>(cp.x) - cam_x;
        int screen_y = static_cast<int>(cp.y) - cam_y;
        
        if (screen_x + cp.width < -50 || screen_x - cp.width > SCREEN_WIDTH + 50 ||
            screen_y + cp.height < -50 || screen_y - cp.height > SCREEN_HEIGHT + 50) {
            continue;
        }
        
        SDL2pp::Rect checkpoint_rect(
            screen_x - cp.width / 2,
            screen_y - cp.height / 2,
            cp.width,
            cp.height
        );
        
        if (cp.type == "start") renderer.SetDrawColor(0, 255, 0, 255);
        else if (cp.type == "finish") renderer.SetDrawColor(255, 0, 0, 255);
        else renderer.SetDrawColor(255, 255, 0, 255);
        
        renderer.DrawRect(checkpoint_rect);
        
        // Bordes gruesos
        SDL2pp::Rect inner_rect1(checkpoint_rect.x + 1, checkpoint_rect.y + 1, checkpoint_rect.w - 2, checkpoint_rect.h - 2);
        renderer.DrawRect(inner_rect1);
        SDL2pp::Rect inner_rect2(checkpoint_rect.x + 2, checkpoint_rect.y + 2, checkpoint_rect.w - 4, checkpoint_rect.h - 4);
        renderer.DrawRect(inner_rect2);
        
        // Indicador numérico
        int center_x = screen_x;
        int center_y = screen_y;
        int circle_radius = 12;
        
        renderer.SetDrawColor(255, 255, 255, 255);
        for (int w = -circle_radius; w <= circle_radius; w++) {
            for (int h = -circle_radius; h <= circle_radius; h++) {
                if ((w*w + h*h) <= circle_radius * circle_radius) {
                    renderer.DrawPoint(center_x + w, center_y + h);
                }
            }
        }
        
        if (cp.type == "start") renderer.SetDrawColor(0, 255, 0, 255);
        else if (cp.type == "finish") renderer.SetDrawColor(255, 0, 0, 255);
        else renderer.SetDrawColor(255, 255, 0, 255);
        
        for (int angle = 0; angle < 360; angle += 1) {
            int x = center_x + circle_radius * std::cos(angle * M_PI / 180.0f);
            int y = center_y + circle_radius * std::sin(angle * M_PI / 180.0f);
            renderer.DrawPoint(x, y);
        }
        
        renderer.SetDrawColor(0, 0, 0, 255);
        if (cp.id < 10) {
            for (int i = 0; i < (cp.id % 10); i++) {
                int offset = i - 2;
                renderer.FillRect(SDL2pp::Rect(center_x + offset * 3, center_y - 1, 2, 2));
            }
        } else {
            renderer.DrawLine(center_x - 5, center_y - 5, center_x + 5, center_y + 5);
            renderer.DrawLine(center_x + 5, center_y - 5, center_x - 5, center_y + 5);
        }
    }
}