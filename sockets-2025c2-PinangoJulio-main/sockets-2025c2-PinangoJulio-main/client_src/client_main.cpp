#include <exception>
#include <iostream>

#include "../common_src/common_constants.h"

#include "client.h"

int main(int argc, const char* argv[]) {
    if (argc != EXPECTED_ARGC_CLIENT) {
        std::cerr << "Usage: " << argv[0] << " <hostname> <port> <commands-file>" << std::endl;
        return ERROR_EXIT_CODE;
    }

    std::string hostname = argv[1];
    std::string port = argv[2];
    std::string commands_file = argv[3];

    try {
        Client client(hostname, port, commands_file);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return ERROR_EXIT_CODE;
    }
    return SUCCESS_EXIT_CODE;
}
