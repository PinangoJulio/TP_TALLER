#include "config.h"

YAML::Node Configuration::yaml;

// Loads the YAML file and stores it in the static 'yaml' variable
void Configuration::load_path(const char *yaml_path) {
    yaml = YAML::LoadFile(yaml_path);
}

// Explicit template instantiations (required when using templates in .cpp files)
template std::string Configuration::get<std::string>(const std::string& field);

template int Configuration::get<int>(const std::string& field);

template float Configuration::get<float>(const std::string& field);

template bool Configuration::get<bool>(const std::string& field);


// Generic template method to get any field from the YAML
template <typename T>
T Configuration::get(const std::string& field) {
    try {
        return yaml[field].as<T>();
    } catch (...) {
        throw std::runtime_error(field + " not found in configuration file.");
    }
}
