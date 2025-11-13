#include "../common_src/config.h" 
#include <box2d/box2d.h>                      
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#include "server.h"

#define ERROR 1
#define SUCCESS 0

int main(int argc, char* argv[]) {
    try {
        // Usar ruta por defecto si no se proporciona argumento
        const char* path_config = (argc >= 2) ? argv[1] : "config.yaml";
        
        std::cout << "==================================================" << std::endl;
        std::cout << "    NEED FOR SPEED 2D - SERVER" << std::endl;
        std::cout << "==================================================" << std::endl;
        std::cout << "Loading configuration from: " << path_config << std::endl;
        
        Configuration::load_path(path_config);
        
        std::string port = Configuration::get<std::string>("port");
        std::cout << "Server will run on port: " << port << std::endl;
        std::cout << "==================================================" << std::endl;
        
        Server server(port.c_str());
        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error initializing the Server: " << e.what() << std::endl;
        return ERROR;
    }
    return SUCCESS;
}
