#include <box2d/box2d.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "../common_src/config.h"
#include "server.h"

#define ERROR            1
#define SUCCESS          0
#define TIPO_AUTO_PRUEBA "DEPORTIVO"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr
            << "Error: Arguments are missing. Correct use : ./server <ruta_a_configuracion.yaml>"
            << std::endl;
        return ERROR;
    }

    try {
        const char* path_config = argv[1];
        Configuration::load_path(path_config);
        Server server((Configuration::get<std::string>("port")).c_str());
        server.start();

    } catch (const std::exception& e) {
        std::cerr << "Error initializing the Server :( " << e.what() << std::endl;
        return ERROR;
    }
    return SUCCESS;
}
