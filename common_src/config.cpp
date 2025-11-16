#include "config.h"
#include <iostream>

YAML::Node Configuration::yaml;

void Configuration::load_path(const char* yaml_path) {
    yaml = YAML::LoadFile(yaml_path);
}

template std::string Configuration::get<std::string>(const std::string& field);
template int Configuration::get<int>(const std::string& field);
template float Configuration::get<float>(const std::string& field);
template bool Configuration::get<bool>(const std::string& field);

template <typename T>
T Configuration::get(const std::string& field) {
    try {
        return yaml[field].as<T>();
    } catch (...) {
        throw std::runtime_error(field + " not found in configuration file.");
    }
}


Configuracion::Configuracion(const std::string& ruta_yaml) {
    Configuration::load_path(ruta_yaml.c_str());
    cargarAtributosAutos();
}

void Configuracion::cargarAtributosAutos() {
    YAML::Node& yaml = get_yaml();
    
    if (!yaml["autos"]) {
        throw std::runtime_error("No 'autos' section found in YAML");
    }
    
    for (const auto& nodo : yaml["autos"]) {
        std::string tipo = nodo["tipo"].as<std::string>();
        AtributosAuto attrs;
        attrs.velocidad_maxima = nodo["velocidad_maxima"].as<float>();
        attrs.aceleracion = nodo["aceleracion"].as<float>();
        attrs.salud_base = nodo["salud_base"].as<int>();
        attrs.masa = nodo["masa"].as<float>();
        attrs.control = nodo["control"].as<float>();
        
        autos[tipo] = attrs;
    }
}

float Configuracion::obtenerGravedadX() const {
    YAML::Node& yaml = get_yaml();
    return yaml["fisica"]["gravedad"]["x"].as<float>();
}

float Configuracion::obtenerGravedadY() const {
    YAML::Node& yaml = get_yaml();
    return yaml["fisica"]["gravedad"]["y"].as<float>();
}

float Configuracion::obtenerTiempoPaso() const {
    YAML::Node& yaml = get_yaml();
    return yaml["fisica"]["tiempo_paso"].as<float>();
}

int Configuracion::obtenerIteracionesVelocidad() const {
    YAML::Node& yaml = get_yaml();
    return yaml["fisica"]["iteraciones_velocidad"].as<int>();
}

int Configuracion::obtenerIteracionesPosicion() const {
    YAML::Node& yaml = get_yaml();
    return yaml["fisica"]["iteraciones_posicion"].as<int>();
}

const AtributosAuto& Configuracion::obtenerAtributosAuto(const std::string& tipo) const {
    auto it = autos.find(tipo);
    if (it == autos.end()) {
        throw std::runtime_error("Tipo de auto '" + tipo + "' no encontrado en configuración");
    }
    return it->second;
}
