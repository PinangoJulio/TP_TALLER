#include "config.h"

#include <sstream>

YAML::Node Configuration::yaml;

// Loads the YAML file and stores it in the static 'yaml' variable
void Configuration::load_path(const char* yaml_path) {
    yaml = YAML::LoadFile(yaml_path);
}

// Generic template method to get any field from the YAML
template <typename T>
T Configuration::get(const std::string& field) {
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

        return node.as<T>();
    } catch (const std::exception& e) {
        throw std::runtime_error("Error reading field '" + field + "': " + e.what());
    }
}

// Get YAML node for complex structures (arrays, maps, etc.)
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
        throw std::runtime_error("Error reading field '" + field + "': " + e.what());
    }
}

// ✅ Ahora sí, recién acá las instanciaciones explícitas
template std::string Configuration::get<std::string>(const std::string& field);
template int Configuration::get<int>(const std::string& field);
template float Configuration::get<float>(const std::string& field);
template bool Configuration::get<bool>(const std::string& field);
