#include <iostream>
#include <ostream>

#include "server.h"

/*int main(int argc, const char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Error. Usage: ./server <servicename>\n";
            return 1;
        }

        std::string servicename = argv[1];
        Server server(servicename);
        server.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Something went wrong starting the server. An exception was caught: "
                  << e.what() << std::endl;
    }
}
*/