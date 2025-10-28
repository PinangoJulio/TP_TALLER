#include <exception>
#include <iostream>

#include "../common_src/common_constants.h"

#include "server.h"

int main(int argc, const char* argv[]) {
    if (argc != EXPECTED_ARGC_SERVER) {
        std::cerr << "Usage: " << argv[0] << " <port> <market-file>" << std::endl;
        return ERROR_EXIT_CODE;
    }

    std::string port = argv[1];
    std::string market_file = argv[2];

    try {
        Server server(port, market_file);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return ERROR_EXIT_CODE;
    }
    return SUCCESS_EXIT_CODE;
}
