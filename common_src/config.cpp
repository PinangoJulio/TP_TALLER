#include "config.h"
#include <stdexcept>
#include <iostream>

// Función auxiliar para extraer datos de un nodo YAML
AtributosAuto cargarAtributosAuto(const YAML::Node& nodo) {
    if (!nodo["VELOCIDAD_MAXIMA"] || !nodo["MASA"] || !nodo["ACELERACION"] || !nodo["SALUD_BASE"] || !nodo["CONTROL"]) {
        throw std::runtime_error("Faltan campos esenciales del auto en YAML.");
    }
    return {
        nodo["VELOCIDAD_MAXIMA"].as<float>(),
        nodo["ACELERACION"].as<float>(),
        nodo["SALUD_BASE"].as<int>(),
        nodo["MASA"].as<float>(),
        nodo["CONTROL"].as<float>()
    };
}

Configuracion::Configuracion(const std::string& ruta_yaml) {
    try {
        // Carga el archivo YAML
        YAML::Node config = YAML::LoadFile(ruta_yaml);

        // 1. Cargar parámetros del mundo
        const YAML::Node& mundo = config["MUNDO"];
        gravedad_x = mundo["GRAVEDAD_X"].as<float>();
        gravedad_y = mundo["GRAVEDAD_Y"].as<float>();
        tiempo_paso_s = mundo["TIEMPO_PASO_S"].as<float>();
        it_velocidad = mundo["ITERACIONES_VELOCIDAD"].as<int>();
        it_posicion = mundo["ITERACIONES_POSICION"].as<int>();

        // 2. Cargar atributos de los autos
        const YAML::Node& autos_node = config["AUTOS"];
        for (YAML::const_iterator it = autos_node.begin(); it != autos_node.end(); ++it) {
            std::string tipo = it->first.as<std::string>();
            autos[tipo] = cargarAtributosAuto(it->second);
        }

    } catch (const YAML::Exception& e) {
        throw std::runtime_error("Error al cargar la configuración YAML: " + std::string(e.what()));
    }
}

const AtributosAuto& Configuracion::obtenerAtributosAuto(const std::string& tipo) const {
    if (autos.count(tipo) == 0) {
        throw std::runtime_error("Tipo de auto no encontrado: " + tipo);
    }
    return autos.at(tipo);
}