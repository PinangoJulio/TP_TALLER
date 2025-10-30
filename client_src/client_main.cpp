#include <iostream>
#include <ostream>

#include "client.h"

int main(int argc, const char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Error. Usage: ./client <hostname> <servicename>\n";
            return 1;
        }

        std::string hostname = argv[1];
        std::string servicename = argv[2];
        Client client(hostname, servicename);
        client.run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Something went wrong starting the client. An exception was caught: "
                  << e.what() << std::endl;
    }
}
