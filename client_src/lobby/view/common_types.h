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

    // Conversión a RaceConfig del servidor (cuando se envíe al servidor)
    // ServerRaceConfig toServerConfig() const {
    //     return {cityName.toStdString(), trackName.toStdString()};
    // }
};

// Alias para compatibilidad con código existente del cliente
using RaceConfig = RaceConfigQt;

#endif  // COMMON_TYPES_H
