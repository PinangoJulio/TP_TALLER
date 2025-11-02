#include "lobby_server.h"
#include "../../common_src/lobby_protocol.h"
#include <iostream>
#include <netinet/in.h>
#include <thread>

LobbyServer::LobbyServer(const std::string& port)
    : acceptor_socket(port.c_str()), running(false) {
    std::cout << "[LobbyServer] Server started on port " << port << std::endl;
}

uint8_t LobbyServer::read_message_type(Socket& socket) {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        throw std::runtime_error("Connection closed");
    }
    return type;
}

std::string LobbyServer::read_string(Socket& socket) {
    uint16_t len_net;
    socket.recvall(&len_net, sizeof(len_net));
    uint16_t len = ntohs(len_net);
    
    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    
    return std::string(buffer.begin(), buffer.end());
}

uint16_t LobbyServer::read_uint16(Socket& socket) {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void LobbyServer::send_buffer(Socket& socket, const std::vector<uint8_t>& buffer) {
    socket.sendall(buffer.data(), buffer.size());
}

void LobbyServer::process_client_messages(Socket& client_socket, const std::string& username) {
    try {
        while (running) {
            uint8_t msg_type = read_message_type(client_socket);
            
            switch (msg_type) {
                case MSG_LIST_GAMES: {
                    std::cout << "[LobbyServer] Client '" << username << "' requested games list" << std::endl;
                    
                    auto games = lobby_manager.get_games_list();
                    auto response = LobbyProtocol::serialize_games_list(games);
                    send_buffer(client_socket, response);
                    break;
                }
                
                case MSG_CREATE_GAME: {
                    std::string game_name = read_string(client_socket);
                    uint8_t max_players;
                    client_socket.recvall(&max_players, sizeof(max_players));
                    
                    std::cout << "[LobbyServer] Client '" << username 
                              << "' creating game: " << game_name << std::endl;
                    
                    uint16_t game_id = lobby_manager.create_game(game_name, max_players);
                    
                    // Agregar al creador al juego
                    lobby_manager.join_game(game_id, username);
                    
                    auto response = LobbyProtocol::serialize_game_created(game_id);
                    send_buffer(client_socket, response);
                    break;
                }
                
                case MSG_JOIN_GAME: {
                    uint16_t game_id = read_uint16(client_socket);
                    
                    std::cout << "[LobbyServer] Client '" << username 
                              << "' joining game: " << game_id << std::endl;
                    
                    bool success = lobby_manager.join_game(game_id, username);
                    
                    if (success) {
                        auto response = LobbyProtocol::serialize_game_joined(game_id);
                        send_buffer(client_socket, response);
                    } else {
                        auto response = LobbyProtocol::serialize_error(
                            ERR_GAME_FULL, "Game is full or already started");
                        send_buffer(client_socket, response);
                    }
                    break;
                }
                
                default:
                    std::cout << "[LobbyServer] Unknown message type: " 
                              << static_cast<int>(msg_type) << std::endl;
                    break;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Client '" << username 
                  << "' disconnected: " << e.what() << std::endl;
    }
}

void LobbyServer::handle_client(Socket client_socket) {
    try {
        // Leer username
        uint8_t msg_type = read_message_type(client_socket);
        if (msg_type != MSG_USERNAME) {
            std::cout << "[LobbyServer] Expected USERNAME message" << std::endl;
            return;
        }
        
        std::string username = read_string(client_socket);
        std::cout << "[LobbyServer] Client connected: " << username << std::endl;
        
        // Enviar bienvenida
        std::string welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        auto welcome_buffer = LobbyProtocol::serialize_welcome(welcome_msg);
        send_buffer(client_socket, welcome_buffer);
        
        // Procesar mensajes del cliente
        process_client_messages(client_socket, username);
        
    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Error handling client: " << e.what() << std::endl;
    }
}

void LobbyServer::run() {
    running = true;
    std::cout << "[LobbyServer] Waiting for connections..." << std::endl;
    
    while (running) {
        try {
            Socket client_socket = acceptor_socket.accept();
            std::cout << "[LobbyServer] New connection accepted" << std::endl;
            
            // Manejar cada cliente en un thread separado
            std::thread client_thread(&LobbyServer::handle_client, this, std::move(client_socket));
            client_thread.detach();
            
        } catch (const std::exception& e) {
            if (running) {
                std::cout << "[LobbyServer] Error accepting connection: " << e.what() << std::endl;
            }
        }
    }
}

void LobbyServer::stop() {
    running = false;
    std::cout << "[LobbyServer] Server stopped" << std::endl;
}
