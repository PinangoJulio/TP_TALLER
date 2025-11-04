#include <iostream>
#include <string>
#include "lobby_client.h"

void print_games(const std::vector<GameInfo>& games) {
    std::cout << "\n========== AVAILABLE GAMES ==========" << std::endl;
    if (games.empty()) {
        std::cout << "No games available." << std::endl;
    } else {
        for (const auto& game : games) {
            std::cout << "[" << game.game_id << "] " << game.game_name 
                      << " (" << static_cast<int>(game.current_players) << "/" 
                      << static_cast<int>(game.max_players) << ")"
                      << (game.is_started ? " [STARTED]" : "") << std::endl;
        }
    }
    std::cout << "=====================================" << std::endl;
}

void show_menu() {
    std::cout << "\n--- LOBBY MENU ---" << std::endl;
    std::cout << "1. List games" << std::endl;
    std::cout << "2. Create game" << std::endl;
    std::cout << "3. Join game" << std::endl;
    std::cout << "4. Exit" << std::endl;
    std::cout << "Choose option: ";
}

int main(int argc, char* argv[]) {
    try {
        std::string host = "localhost";
        std::string port = "8080";
        
        if (argc >= 3) {
            host = argv[1];
            port = argv[2];
        }
        
        std::cout << "==================================================" << std::endl;
        std::cout << "    NEED FOR SPEED 2D - LOBBY CLIENT (TEST)" << std::endl;
        std::cout << "==================================================" << std::endl;
        
        // Pedir username
        std::string username;
        std::cout << "Enter your username: ";
        std::getline(std::cin, username);
        
        // Conectar al servidor
        LobbyClient client(host, port);
        
        // Enviar username
        client.send_username(username);
        
        // Recibir bienvenida
        std::string welcome = client.receive_welcome();
        std::cout << "\n" << welcome << "\n" << std::endl;
        
        // Menú interactivo
        bool running = true;
        while (running && client.is_connected()) {
            show_menu();
            
            int option;
            std::cin >> option;
            std::cin.ignore(); // Limpiar buffer
            
            switch (option) {
                case 1: { // List games
                    client.request_games_list();
                    auto games = client.receive_games_list();
                    print_games(games);
                    break;
                }
                
                case 2: { // Create game
                    std::string game_name;
                    int max_players;
                    
                    std::cout << "Game name: ";
                    std::getline(std::cin, game_name);
                    
                    std::cout << "Max players (2-8): ";
                    std::cin >> max_players;
                    std::cin.ignore();
                    
                    client.create_game(game_name, max_players);
                    uint16_t game_id = client.receive_game_created();
                    
                    std::cout << "\n✅ Game created! ID: " << game_id << std::endl;
                    std::cout << "Waiting for other players to join..." << std::endl;
                    break;
                }
                
                case 3: { // Join game
                    int game_id;
                    std::cout << "Game ID to join: ";
                    std::cin >> game_id;
                    std::cin.ignore();
                    
                    client.join_game(game_id);
                    
                    // Leer el tipo de mensaje de respuesta
                    uint8_t msg_type = client.peek_message_type();
                    
                    if (msg_type == MSG_ERROR) {
                        std::string error_msg;
                        client.read_error_details(error_msg);
                        std::cout << "\n❌ Error: " << error_msg << std::endl;
                    } else if (msg_type == MSG_GAME_JOINED) {
                        uint16_t joined_id = client.read_uint16();
                        std::cout << "\n✅ Joined game " << joined_id << "!" << std::endl;
                    } else {
                        std::cout << "\n❌ Unexpected message type: " 
                                  << static_cast<int>(msg_type) << std::endl;
                    }
                    break;
                }
                
                case 4: // Exit
                    running = false;
                    std::cout << "Goodbye!" << std::endl;
                    break;
                
                default:
                    std::cout << "Invalid option." << std::endl;
                    break;
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }
}
