#include "game_renderer.h"
#include <iostream>
#include <cmath>
#include <stdexcept>
#include <SDL2/SDL_image.h> 
#include <SDL2pp/Surface.hh>

// Define el tamaño de pantalla aquí o pásalo en el constructor si varía
const int SCREEN_WIDTH = 1280; 
const int SCREEN_HEIGHT = 720;

GameRenderer::GameRenderer(SDL2pp::Renderer& r, const RaceInfoDTO& info)
    : renderer(r), race_info(info), map_width(0), map_height(0) {
    
    // Inicializar clips
    // Hardcodeado temporal 
    car_clips[0] = SDL2pp::Rect(32, 0, 32, 32);  // Arriba
    car_clips[7] = SDL2pp::Rect(0, 0, 32, 32);   // Diag
    
    init();
}

void GameRenderer::init() {
    load_textures();
}

// Función helper para cargar Surface de forma segura usando IMG_Load
// Esto evita el error de "no matching function" si SDL2pp no detectó SDL_image
SDL2pp::Surface load_surface_safe(const std::string& path) {
    SDL_Surface* raw_surf = IMG_Load(path.c_str());
    if (!raw_surf) {
        throw std::runtime_error("Error cargando imagen '" + path + "': " + std::string(IMG_GetError()));
    }
    // SDL2pp::Surface toma posesión del puntero raw_surf y lo liberará automáticamente
    return SDL2pp::Surface(raw_surf);
}

void GameRenderer::load_textures() {
    // 1. Cargar Mapa 
    std::string base_path = "assets/img/map/cities/";
    std::string city_file = base_path + "vice-city.png"; // Default
    
    // Ajustar nombre de archivo según la ciudad recibida
    if (std::string(race_info.city_name) == "San Andreas") {
        city_file = base_path + "san-andreas.png";
    } else if (std::string(race_info.city_name) == "Liberty City") {
        city_file = base_path + "liberty-city.png";
    }

    try {
        // CORRECCIÓN: Usar IMG_Load a través del helper
        SDL2pp::Surface map_surf = load_surface_safe(city_file);
        map_texture = std::make_unique<SDL2pp::Texture>(renderer, map_surf);
        
        map_width = map_texture->GetWidth();
        map_height = map_texture->GetHeight();

        // Minimapa (Usamos la misma imagen por ahora)
        minimap_texture = std::make_unique<SDL2pp::Texture>(renderer, map_surf);

        // Autos
        SDL2pp::Surface car_surf = load_surface_safe("assets/img/map/cars/spriteshit-cars.png");
        car_textures["sport"] = std::make_unique<SDL2pp::Texture>(renderer, car_surf);
        // Cargar resto de autos...

        // Capas extra (Puentes/Top)
        std::string bridges_path = "assets/img/map/layers/vice-city/puentes-vice-city.png";
        std::string top_path = "assets/img/map/layers/vice-city/vice-city-top.png";

        if (std::string(race_info.city_name) == "Vice City") {
             // Solo intentamos cargar capas extra si estamos en Vice City por ahora
             try {
                SDL2pp::Surface bridge_surf = load_surface_safe(bridges_path);
                puentes_texture = std::make_unique<SDL2pp::Texture>(renderer, bridge_surf);
                
                SDL2pp::Surface top_surf = load_surface_safe(top_path);
                top_texture = std::make_unique<SDL2pp::Texture>(renderer, top_surf);
             } catch (const std::exception& e) {
                std::cout << "[Renderer] Info: Capas extra no encontradas (" << e.what() << ")" << std::endl;
             }
        }

    } catch (const std::exception& e) {
        std::cerr << "[Renderer] Error fatal cargando texturas: " << e.what() << std::endl;
       
    }
}

void GameRenderer::render(const GameState& state, int my_player_id) {
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();

    if (!map_texture) return; // Si falló la carga, no renderizamos nada para evitar crash

    // 1. ENCONTRAR AL JUGADOR LOCAL (Para centrar la cámara)
    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == my_player_id) {
            local_player = &p;
            break;
        }
    }

    if (!local_player) {
        renderer.Present();
        return; 
    }

    // 2. CÁLCULO DE CÁMARA (Viewport)
    SDL2pp::Rect viewport(
        static_cast<int>(local_player->pos_x - SCREEN_WIDTH / 2),
        static_cast<int>(local_player->pos_y - SCREEN_HEIGHT / 2),
        SCREEN_WIDTH,
        SCREEN_HEIGHT
    );

    // Clamping 
    if (viewport.x < 0) viewport.x = 0;
    if (viewport.y < 0) viewport.y = 0;
    if (viewport.x + viewport.w > map_width) viewport.x = map_width - viewport.w;
    if (viewport.y + viewport.h > map_height) viewport.y = map_height - viewport.h;

    // 3. DIBUJAR MAPA BASE
    SDL2pp::Rect screen(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    renderer.Copy(*map_texture, viewport, screen);

    // 4. DIBUJAR PUENTES
    if (puentes_texture) {
         renderer.Copy(*puentes_texture, viewport, screen);
    }

    // 5. DIBUJAR TODOS LOS AUTOS
    for (const auto& player : state.players) {
        render_car(player, viewport);
    }

    // 6. DIBUJAR CAPAS SUPERIORES
    if (top_texture) {
        renderer.Copy(*top_texture, viewport, screen);
    }

    // 7. MINIMAPA
    render_minimap(*local_player);

    renderer.Present();
}

void GameRenderer::render_car(const InfoPlayer& player, const SDL2pp::Rect& viewport) {
    if (!player.is_alive) return;

    // Seleccionar textura
    std::string type = player.car_type.empty() ? "sport" : player.car_type;
    auto it = car_textures.find(type);
    
    // Si no encuentra la específica, usa "sport" por defecto
    if (it == car_textures.end()) it = car_textures.find("sport");
    if (it == car_textures.end()) return; 

    // Calcular Posición en Pantalla relativo al Viewport
    int screen_x = static_cast<int>(player.pos_x) - viewport.x - 16; 
    int screen_y = static_cast<int>(player.pos_y) - viewport.y - 16;

    // Seleccionar Clip según ángulo
    int clip_idx = get_clip_index_from_angle(player.angle);
    
    SDL2pp::Rect clip = car_clips.count(clip_idx) ? car_clips[clip_idx] : SDL2pp::Rect(0,0,32,32);
    SDL2pp::Rect dest(screen_x, screen_y, 32, 32);

    renderer.Copy(*it->second, clip, dest, 0.0); 
}

void GameRenderer::render_minimap(const InfoPlayer& player) {
    if (!minimap_texture) return;

    int minimapSrcX = static_cast<int>(player.pos_x) - (MINIMAP_SCOPE / 2);
    int minimapSrcY = static_cast<int>(player.pos_y) - (MINIMAP_SCOPE / 2);

    // Clamping minimapa
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

    // Fondo Negro
    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.FillRect(minimapDest);

    // Mapa
    renderer.Copy(*minimap_texture, minimapSrc, minimapDest);

    // Borde Blanco
    renderer.SetDrawColor(255, 255, 255, 255);
    renderer.DrawRect(minimapDest);

    // Dibujar Puntito del Jugador
    float scale = (float)MINIMAP_SIZE / (float)MINIMAP_SCOPE;
    float playerRelX = player.pos_x - minimapSrcX;
    float playerRelY = player.pos_y - minimapSrcY;

    int dotX = minimapDest.x + (playerRelX * scale);
    int dotY = minimapDest.y + (playerRelY * scale);
    
    // Borde del punto
    renderer.SetDrawColor(255, 255, 255, 255);
    renderer.FillRect(SDL2pp::Rect(dotX - 4, dotY - 4, 8, 8));

    // Interior del punto (Cyan)
    renderer.SetDrawColor(0, 255, 255, 255);
    renderer.FillRect(SDL2pp::Rect(dotX - 3, dotY - 3, 6, 6));
}

int GameRenderer::get_clip_index_from_angle(float angle) {
    // CORRECCIÓN 2: Suprimir advertencia de parámetro no usado explícitamente
    (void)angle; 
    
    // TODO: Implementar lógica real trigonométrica para devolver 0..7
    return 0; 
}