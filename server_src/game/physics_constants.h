#ifndef PHYSICS_CONSTANTS_H
#define PHYSICS_CONSTANTS_H

// Escala: 30 píxeles = 1 metro
constexpr float PIXELS_PER_METER = 30.0f;

constexpr float CAR_SMALL_SIZE = 32.0f;   // 32x32
constexpr float CAR_MEDIUM_SIZE = 40.0f;  // 40x40
constexpr float CAR_LARGE_SIZE = 50.0f;   // 50x50

// Funciones de conversión
inline float pixelsToMeters(float pixels) {
    return pixels * METERS_PER_PIXEL;
}

inline float metersToPixels(float meters) {
    return meters * PIXELS_PER_METER;
}

#endif // PHYSICS_CONSTANTS_H