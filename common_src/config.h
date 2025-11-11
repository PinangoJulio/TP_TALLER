#ifndef CONFIG_H
#define CONFIG_H


#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

class Configuration {
private:
    static YAML::Node yaml;

public:
    static void load_path(const char *yaml_path);

    // Generic getter for any configuration field
    template <typename T>
    static T get(const std::string& field);
};
#endif //CONFIG_H
