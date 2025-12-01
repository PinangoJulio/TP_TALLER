#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <map>
#include <vector>
#include <string>
#include "common_src/game_state.h"

class GameRenderer {
private:
    SDL2pp::Renderer& renderer;

    // Texturas del juego
    SDL2pp::Texture map_texture;
    SDL2pp::Texture car_texture;

    // Clips para las animaciones del auto (mapeo de índice -> rect)
    std::map<int, SDL2pp::Rect> car_clips;

    // Dimensiones
    int screen_width;
    int screen_height;
    int map_width;
    int map_height;

    
    int getClipIndexFromAngle(float angle_radians);

public:

    GameRenderer(SDL2pp::Renderer& renderer, int screen_w, int screen_h);

    
    void render(const GameState& state, int player_id);

    ~GameRenderer() = default;
};

#endif // GAME_RENDERER_H