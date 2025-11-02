#include <iostream>
#include <string>
#include "lobby/lobby_server.h"

int main(int argc, char* argv[]) {
    try {
        std::string port = "8080";
        
        if (argc >= 2) {
            port = argv[1];
        }
        
        std::cout << "==================================================" << std::endl;
        std::cout << "    NEED FOR SPEED 2D - LOBBY SERVER (TEST)" << std::endl;
        std::cout << "==================================================" << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;
        std::cout << "==================================================" << std::endl;
        
        LobbyServer server(port);
        server.run();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[FATAL] Server error: " << e.what() << std::endl;
        return 1;
    }
}
