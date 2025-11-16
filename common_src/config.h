#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>
#include <map>

// Estructura para atributos de autos (Box2D)
struct AtributosAuto {
    float velocidad_maxima;
    float aceleracion;
    int salud_base;
    float masa;
    float control;
};

class Configuration {
private:
    static YAML::Node yaml;

public:
    static void load_path(const char* yaml_path);
    template <typename T>
    static T get(const std::string& field);
    
protected:
    static YAML::Node& get_yaml() { return yaml; }
};

// Clase extendida para Box2D (mantiene compatibilidad)
class Configuracion : public Configuration {
private:
    std::map<std::string, AtributosAuto> autos;
    
    void cargarAtributosAutos();

public:
    explicit Configuracion(const std::string& ruta_yaml);
    
    float obtenerGravedadX() const;
    float obtenerGravedadY() const;
    float obtenerTiempoPaso() const;
    int obtenerIteracionesVelocidad() const;
    int obtenerIteracionesPosicion() const;
    
    const AtributosAuto& obtenerAtributosAuto(const std::string& tipo) const;
};

#endif
