#ifndef CHECKPOINT_MANAGER_H
#define CHECKPOINT_MANAGER_H

#include <vector>
#include <map>
#include <box2d/box2d.h>
#include "map_loader.h"

class CheckpointManager {
private:
    std::vector<CheckpointData> checkpoints;
    std::map<int, int> player_current_checkpoint;
    std::map<int, int> player_laps_completed;
    
    bool is_point_in_checkpoint(b2Vec2 point, const CheckpointData& cp);

public:
    CheckpointManager() = default;
    
    void load_checkpoints(const std::vector<CheckpointData>& cps);
    void register_player(int player_id);
    bool check_crossing(int player_id, b2Vec2 car_position);
    int get_current_checkpoint(int player_id) const;
    int get_laps_completed(int player_id) const;
    bool has_completed_lap(int player_id) const;
    int get_total_checkpoints() const { return checkpoints.size(); }
};

#endif
