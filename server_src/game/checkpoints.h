#ifndef CHECKPOINTS_H
#define CHECKPOINTS_H

#include <string>

// Copiamos EXACTAMENTE la estructura que usan tus compañeros en GameLoop
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