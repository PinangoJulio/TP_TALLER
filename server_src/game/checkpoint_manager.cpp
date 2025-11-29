#include "checkpoint_manager.h"
#include <iostream>
#include <cmath>

void CheckpointManager::load_checkpoints(const std::vector<CheckpointData>& cps) {
    checkpoints = cps;
    std::cout << "[CheckpointManager] Loaded " << checkpoints.size() << " checkpoints" << std::endl;
}

void CheckpointManager::register_player(int player_id) {
    player_current_checkpoint[player_id] = 0;
    player_laps_completed[player_id] = 0;
    std::cout << "[CheckpointManager] Player " << player_id << " registered" << std::endl;
}

bool CheckpointManager::is_point_in_checkpoint(b2Vec2 point, const CheckpointData& cp) {
    float half_width = cp.width / 2.0f;
    float half_height = cp.height / 2.0f;
    
    bool inside_x = (point.x >= cp.x - half_width) && (point.x <= cp.x + half_width);
    bool inside_y = (point.y >= cp.y - half_height) && (point.y <= cp.y + half_height);
    
    return inside_x && inside_y;
}

bool CheckpointManager::check_crossing(int player_id, b2Vec2 car_position) {
    if (checkpoints.empty()) return false;
    
    auto it = player_current_checkpoint.find(player_id);
    if (it == player_current_checkpoint.end()) {
        std::cerr << "[CheckpointManager] Player " << player_id << " not registered!" << std::endl;
        return false;
    }
    
    int current_cp_index = it->second;
    const CheckpointData& next_cp = checkpoints[current_cp_index];
    
    if (is_point_in_checkpoint(car_position, next_cp)) {
        std::cout << "[CheckpointManager] Player " << player_id 
                  << " crossed checkpoint " << next_cp.id << std::endl;
        
        current_cp_index++;
        
        if (current_cp_index >= static_cast<int>(checkpoints.size())) {
            current_cp_index = 0;
            player_laps_completed[player_id]++;
            
            std::cout << "[CheckpointManager] 🏁 Player " << player_id 
                      << " completed lap " << player_laps_completed[player_id] << "!" << std::endl;
            
            player_current_checkpoint[player_id] = current_cp_index;
            return true;
        }
        
        player_current_checkpoint[player_id] = current_cp_index;
    }
    
    return false;
}

int CheckpointManager::get_current_checkpoint(int player_id) const {
    auto it = player_current_checkpoint.find(player_id);
    return (it != player_current_checkpoint.end()) ? it->second : -1;
}

int CheckpointManager::get_laps_completed(int player_id) const {
    auto it = player_laps_completed.find(player_id);
    return (it != player_laps_completed.end()) ? it->second : 0;
}

bool CheckpointManager::has_completed_lap(int player_id) const {
    return get_laps_completed(player_id) > 0;
}
