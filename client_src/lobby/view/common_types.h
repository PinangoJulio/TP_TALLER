#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>

// Struct para configuración de carreras
struct RaceConfig {
    QString cityName;
    int trackIndex;
    QString trackName;

};

struct PlayerResult {
    QString playerName;
    int rank;           // 1, 2, 3...
    QString totalTime;  // Ej: "02:45.33"
    int score;          // Puntos acumulados (Tenemos puntos???)
    QString carName;    
};

#endif // COMMON_TYPES_H