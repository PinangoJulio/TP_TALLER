#include "game_renderer.h"
#include <iostream>
#include <cmath>
#include <SDL_image.h> 

// Factor de conversión de radianes a grados
const float RAD_TO_DEG = 180.0f / M_PI;

GameRenderer::GameRenderer(SDL2pp::Renderer& renderer_ref, int screen_w, int screen_h)
    : renderer(renderer_ref),
      //IMAGEN DE PREPO,ESTO DEBERÍA VARIAR CON EL YAML DEL MOMENTO
      map_texture(renderer, SDL2pp::Surface(IMG_Load("assets/img/map/cities/vice-city.png"))), 
      car_texture(renderer, SDL2pp::Surface(IMG_Load("assets/img/map/cars/spriteshit-cars.png"))),
      screen_width(screen_w),
      screen_height(screen_h) {

    // Inicializar clips del sprite sheet 
    car_clips[0] = SDL2pp::Rect(32, 0, 32, 32);   // Arriba
    car_clips[1] = SDL2pp::Rect(64, 0, 32, 32);   // Arriba-Derecha
    car_clips[2] = SDL2pp::Rect(96, 0, 32, 32);   // Derecha
    car_clips[3] = SDL2pp::Rect(128, 0, 32, 32);  // Abajo-Derecha
    car_clips[4] = SDL2pp::Rect(160, 0, 32, 32);  // Abajo
    car_clips[5] = SDL2pp::Rect(192, 0, 32, 32);  // Abajo-Izquierda
    car_clips[6] = SDL2pp::Rect(224, 0, 32, 32);  // Izquierda
    car_clips[7] = SDL2pp::Rect(0, 0, 32, 32);    // Arriba-Izquierda

    // Obtener dimensiones reales del mapa cargado
    map_width = map_texture.GetWidth();
    map_height = map_texture.GetHeight();
}

int GameRenderer::getClipIndexFromAngle(float angle_radians) {
    float degrees = angle_radians * RAD_TO_DEG;
    
    
    while (degrees < 0) degrees += 360;
    while (degrees >= 360) degrees -= 360;

   
    int index = static_cast<int>((degrees + 22.5f) / 45.0f); 
    return (index + 2) % 8;
}

void GameRenderer::render(const GameState& state, int player_id) {
    // Encontrar al jugador local para centrar la cámara
    const InfoPlayer* local_player = nullptr;
    for (const auto& p : state.players) {
        if (p.player_id == player_id) {
            local_player = &p;
            break;
        }
    }

    int cam_x = 0;
    int cam_y = 0;

    if (local_player) {
        // Centrar: Posición Jugador - Mitad de Pantalla
        cam_x = static_cast<int>(local_player->pos_x) - screen_width / 2;
        cam_y = static_cast<int>(local_player->pos_y) - screen_height / 2;
    }

    // Clamping: Evitar que la cámara se salga del mapa
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > map_width - screen_width) cam_x = map_width - screen_width;
    if (cam_y > map_height - screen_height) cam_y = map_height - screen_height;

    SDL2pp::Rect viewport(cam_x, cam_y, screen_width, screen_height);
    SDL2pp::Rect screen_rect(0, 0, screen_width, screen_height);

    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();

    // Renderizar Mapa
    renderer.Copy(map_texture, viewport, screen_rect);

    // Renderizar Jugadores
    for (const auto& player : state.players) {
      
        int screen_x = static_cast<int>(player.pos_x) - cam_x;
        int screen_y = static_cast<int>(player.pos_y) - cam_y;

        int clip_idx = getClipIndexFromAngle(player.angle);
        
       
        if (car_clips.find(clip_idx) == car_clips.end()) clip_idx = 0;
        SDL2pp::Rect clip = car_clips[clip_idx];

       
        SDL2pp::Rect dest(screen_x - clip.w / 2, screen_y - clip.h / 2, clip.w, clip.h);

        renderer.Copy(car_texture, clip, dest);
    }

    renderer.Present();
}