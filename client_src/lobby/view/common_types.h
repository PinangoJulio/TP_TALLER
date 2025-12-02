#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <QString>

// Nota: Las estructuras del servidor están en game_state.h
// Este archivo contiene tipos específicos del cliente Qt

// Configuración de carrera para la UI de Qt
struct RaceConfigQt {
    QString cityName;
    int trackIndex;
    QString trackName;

   
};


using RaceConfig = RaceConfigQt;

#endif  // COMMON_TYPES_H
