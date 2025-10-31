#ifndef CONFIG_H
#define CONFIG_H

#pragma once
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>

// Estructura para contener los atributos de un tipo de auto
struct AtributosAuto {
    float velocidad_maxima;
    float aceleracion;
    int salud_base;
    float masa;
    float control;
};

class Configuracion {
private:
    float gravedad_x;
    float gravedad_y;
    float tiempo_paso_s;
    int it_velocidad;
    int it_posicion;
    std::map<std::string, AtributosAuto> autos;

public:
    // Constructor que lanza excepción si el archivo no se carga.
    Configuracion(const std::string& ruta_yaml);

    // Métodos para Box2D
    float obtenerGravedadX() const { return gravedad_x; }
    float obtenerGravedadY() const { return gravedad_y; }
    float obtenerTiempoPaso() const { return tiempo_paso_s; }
    int obtenerIteracionesVelocidad() const { return it_velocidad; }
    int obtenerIteracionesPosicion() const { return it_posicion; }

    // Métodos para los autos
    const AtributosAuto& obtenerAtributosAuto(const std::string& tipo) const;
};

#endif //CONFIG_H