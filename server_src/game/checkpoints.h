#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H
#include <string>

struct Checkpoint {
    int id;
    std::string type; 
    float x, y;
    float width, height;
    float angle;
};

struct SpawnPoint {
    float x, y, angle;
};

#endif 
