#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <yaml-cpp/yaml.h>

class Configuration {
private:
    static YAML::Node yaml;

public:
    static void load_path(const char* yaml_path);

    template <typename T>
    static T get(const std::string& field);

    static YAML::Node get_node(const std::string& field);
};

#endif // CONFIG_H