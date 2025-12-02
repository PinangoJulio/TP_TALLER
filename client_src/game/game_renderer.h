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

    // Clips de animación por tamaño
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_32;
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_40;
    std::map<int, std::map<int, SDL2pp::Rect>> car_clips_50;
    
    struct CarInfo {
        int texture_id;
        int row;
        int size;
    };
    
    std::map<std::string, CarInfo> car_info_map;

    int map_width;
    int map_height;

    // ═══════════════════════════════════════════════════════════
    // NUEVAS ESTRUCTURAS Y DECLARACIONES (Checkpoints)
    // ═══════════════════════════════════════════════════════════
    struct Checkpoint {
        int id;
        std::string type;  // "start", "normal", "finish"
        float x;
        float y;
        float width;
        float height;
        float angle;
    };
    
    std::vector<Checkpoint> checkpoints;
    
    struct SpawnPoint {
        float x;
        float y;
        float angle;
    };
    
    std::vector<SpawnPoint> spawn_points;

    // Funciones auxiliares privadas
    int getClipIndexFromAngle(float angle_radians);
    void load_checkpoints_from_yaml(const std::string& yaml_path);
    void render_checkpoints(const SDL2pp::Rect& viewport, int cam_x, int cam_y);

public:
    static const int SCREEN_WIDTH = 700;
    static const int SCREEN_HEIGHT = 700;

    static const int MINIMAP_SIZE = 200;   
    static const int MINIMAP_MARGIN = 20;  
    static const int MINIMAP_SCOPE = 1000; 

    explicit GameRenderer(SDL2pp::Renderer& renderer);

    void init_race(const std::string& yaml_path);

    void render(const GameState& state, int player_id);

    ~GameRenderer() = default;
};

#endif // GAME_RENDERER_H