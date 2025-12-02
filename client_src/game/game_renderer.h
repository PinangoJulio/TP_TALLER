#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include <SDL2pp/SDL2pp.hh>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include "../../common_src/game_state.h"
#include "collision_manager.h" 

class GameRenderer {
private:
    SDL2pp::Renderer& renderer;

    // Texturas del mapa
    std::unique_ptr<SDL2pp::Texture> map_texture;
    std::unique_ptr<SDL2pp::Texture> puentes_texture;
    std::unique_ptr<SDL2pp::Texture> top_texture;
    
    // Texturas de autos por tamaño
    std::unique_ptr<SDL2pp::Texture> car_texture_32;
    std::unique_ptr<SDL2pp::Texture> car_texture_40;
    std::unique_ptr<SDL2pp::Texture> car_texture_50;
    
    // Textura del Minimapa
    std::unique_ptr<SDL2pp::Texture> minimap_texture;

    // Collision Manager para lógica visual
    std::unique_ptr<CollisionManager> collision_manager;

    // Clips de animación por tamaño: [fila][dirección] → SDL2pp::Rect
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_32;
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_40;
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_50;
    
    // Estructura para info de cada auto
    struct CarInfo {
        int texture_id;  // 0=32px, 1=40px, 2=50px
        int row;         // Fila en su spritesheet
        int size;        // Tamaño del sprite
    };
    
    // Mapper: nombre del auto → info del auto
    std::map<std::string, CarInfo> car_info_map;

    // Dimensiones de la IMAGEN del mapa (se llenan al cargar la carrera)
    int map_width;
    int map_height;

    int getClipIndexFromAngle(float angle_radians);

public:
    // Constantes fijas de pantalla (700x700)
    static const int SCREEN_WIDTH = 700;
    static const int SCREEN_HEIGHT = 700;

    // Constantes del Minimapa
    static const int MINIMAP_SIZE = 200;   
    static const int MINIMAP_MARGIN = 20;  
    static const int MINIMAP_SCOPE = 1000; 

    explicit GameRenderer(SDL2pp::Renderer& renderer);

    // Carga los assets específicos de la carrera leyendo el YAML
    void init_race(const std::string& yaml_path);

    // Dibuja el estado actual
    void render(const GameState& state, int player_id);

    ~GameRenderer() = default;
};

#endif // GAME_RENDERER_H