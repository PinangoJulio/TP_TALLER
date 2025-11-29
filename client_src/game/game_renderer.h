#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <map>
#include <cmath>
#include <memory>
#include <string>
#include <iostream>
#include <SDL2pp/SDL2pp.hh>


#include "../../common_src/dtos.h"
#include "../../common_src/game_state.h"

class GameRenderer {
private:
    SDL2pp::Renderer& renderer;
    RaceInfoDTO race_info;

    // Texturas
    std::unique_ptr<SDL2pp::Texture> map_texture;
    std::unique_ptr<SDL2pp::Texture> minimap_texture; // Textura pequeña para minimapa
    std::map<std::string, std::unique_ptr<SDL2pp::Texture>> car_textures;
    
    // Capas extra (Puentes, Top)
    std::unique_ptr<SDL2pp::Texture> puentes_texture;
    std::unique_ptr<SDL2pp::Texture> top_texture;

    // Dimensiones
    int map_width;
    int map_height;
    
    // Configuración Minimapa 
    static const int MINIMAP_SIZE = 200;
    static const int MINIMAP_MARGIN = 20;
    static const int MINIMAP_SCOPE = 1000;

    // Clips de autos (para rotación simulada si usas spritesheet)
    std::map<int, SDL2pp::Rect> car_clips;

public:
    GameRenderer(SDL2pp::Renderer& renderer, const RaceInfoDTO& info);
    void init(); // Cargar texturas
    void render(const GameState& state, int my_player_id);

private:
    void load_textures();
    void render_minimap(const InfoPlayer& local_player);
    void render_car(const InfoPlayer& player, const SDL2pp::Rect& viewport);
    
    // Helper para convertir ángulo (grados/radianes) a índice del sprite, no sé si lo use todavía
    int get_clip_index_from_angle(float angle);
};

#endif