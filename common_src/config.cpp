#include "config.h"
#include <sstream>

YAML::Node Configuration::yaml;

// Carga el archivo
void Configuration::load_path(const char* yaml_path) {
    try {
        yaml = YAML::LoadFile(yaml_path);
    } catch (const std::exception& e) {
        throw std::runtime_error("Error loading config file: " + std::string(yaml_path));
    }
}

// Implementación del getter genérico
template <typename T>
T Configuration::get(const std::string& field) {
    try {
        YAML::Node node = yaml;
        std::stringstream ss(field);
        std::string key;

        // Navegar por la estructura "padre.hijo.nieto"
        while (std::getline(ss, key, '.')) {
            if (!node[key]) {
                throw std::runtime_error("Field not found: " + field);
            }
            node = node[key];
        }

        return node.as<T>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Error reading config field '" + field + "': " + e.what());
    }
}

// Getter de nodo crudo
YAML::Node Configuration::get_node(const std::string& field) {
    try {
        YAML::Node node = yaml;
        std::stringstream ss(field);
        std::string key;

        while (std::getline(ss, key, '.')) {
            if (!node[key]) {
                throw std::runtime_error("Field not found: " + field);
            }
            node = node[key];
        }

        return node;
    } catch (const std::exception& e) {
        throw std::runtime_error("Error reading config node '" + field + "': " + e.what());
    }
}

// Instanciaciones explícitas para que el linker no se queje
template std::string Configuration::get<std::string>(const std::string& field);
template int Configuration::get<int>(const std::string& field);
template float Configuration::get<float>(const std::string& field);
template bool Configuration::get<bool>(const std::string& field);