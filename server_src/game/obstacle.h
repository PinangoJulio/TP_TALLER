#ifndef OBSTACLE_H
#define OBSTACLE_H

#include <box2d/box2d.h>
#include <string>
#include <vector>

enum class ObstacleType {
    WALL,           // Pared/edificio
    BARRIER,        // Barrera de pista
    BRIDGE_PILLAR   // Pilar de puente
};

struct Obstacle {
    ObstacleType type;
    b2BodyId body;
    float damage_multiplier;  // Multiplicador de daño (paredes = 1.5x)
    
    Obstacle(ObstacleType t, b2BodyId b, float dmg_mult = 1.0f)
        : type(t), body(b), damage_multiplier(dmg_mult) {}
};

class ObstacleManager {
private:
    std::vector<Obstacle> obstacles;
    b2WorldId world;

public:
    explicit ObstacleManager(b2WorldId world_id);
    
    // Crear diferentes tipos de obstáculos
    void create_wall(float x, float y, float width, float height);
    void create_barrier(float x, float y, float length);
    void create_building(float x, float y, float width, float height);
    
    // Verificar si un cuerpo es un obstáculo
    bool is_obstacle(b2BodyId body) const;
    
    // Obtener multiplicador de daño del obstáculo
    float get_damage_multiplier(b2BodyId body) const;
    
    // Limpiar todos los obstáculos
    void clear();
    
    ~ObstacleManager();
};

#endif // OBSTACLE_H
