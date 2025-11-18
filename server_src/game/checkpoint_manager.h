#ifndef CHECKPOINT_MANAGER_H
#define CHECKPOINT_MANAGER_H

#include <vector>
#include <map>
#include <box2d/box2d.h>
#include "map_loader.h"

class CheckpointManager {
private:
    std::vector<CheckpointData> checkpoints;
    
    // Tracking: player_id → checkpoint actual (índice)
    std::map<int, int> player_current_checkpoint;
    
    // Tracking: player_id → vueltas completadas
    std::map<int, int> player_laps_completed;
    
    // Verifica si un punto está dentro de un rectángulo rotado
    bool is_point_in_checkpoint(b2Vec2 point, const CheckpointData& cp);

public:
    CheckpointManager() = default;
    
    // Inicializar con los checkpoints del mapa
    void load_checkpoints(const std::vector<CheckpointData>& cps);
    
    // Registrar un jugador (empieza en checkpoint 0)
    void register_player(int player_id);
    
    // Verificar si el jugador cruzó el siguiente checkpoint
    bool check_crossing(int player_id, b2Vec2 car_position);
    
    // Obtener el checkpoint actual del jugador
    int get_current_checkpoint(int player_id) const;
    
    // Obtener vueltas completadas
    int get_laps_completed(int player_id) const;
    
    // Verificar si completó una vuelta
    bool has_completed_lap(int player_id) const;
    
    // Total de checkpoints
    int get_total_checkpoints() const { return checkpoints.size(); }
};

#endif
