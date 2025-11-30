#ifndef CONFIG_H
#define CONFIG_H

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

class Configuration {
private:
    static YAML::Node yaml;

public:
    // Carga el archivo YAML en memoria
    static void load_path(const char* yaml_path);

    // Método genérico para obtener valores (Ej: get<int>("port"))
    template <typename T>
    static T get(const std::string& field);

    // Obtener un nodo complejo (para listas o estructuras anidadas)
    static YAML::Node get_node(const std::string& field);
};

#endif