#include "lobby_server.h"

#include <netinet/in.h>

#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>

LobbyServer::LobbyServer(const std::string& port) : acceptor_socket(port.c_str()), running(true) {
    std::cout << "[LobbyServer] Server started on port " << port << std::endl;
}

void LobbyServer::run() {
    std::cout << "[LobbyServer] Waiting for connections..." << std::endl;

    while (running) {
        try {
            Socket client_socket = acceptor_socket.accept();
            std::cout << "[LobbyServer] New connection accepted" << std::endl;

            std::thread client_thread(&LobbyServer::handle_client, this, std::move(client_socket));
            client_thread.detach();

        } catch (const std::exception& e) {
            if (running) {
                std::cerr << "[LobbyServer] Error accepting connection: " << e.what() << std::endl;
            }
        }
    }
}

void LobbyServer::handle_client(Socket client_socket) {
    std::cout << "[LobbyServer] DEBUG: Starting to handle new client" << std::endl;

    std::string username;

    try {
        username = receive_username(client_socket);
        std::cout << "[LobbyServer] Client connected: " << username << std::endl;

        send_welcome(client_socket, username);

        bool client_connected = true;
        while (client_connected) {
            std::cout << "[LobbyServer] DEBUG: Waiting to read message type..." << std::endl;
            uint8_t msg_type = read_message_type(client_socket);
            std::cout << "[LobbyServer] DEBUG: Received message type: " << static_cast<int>(msg_type)
                      << std::endl;

            switch (msg_type) {
            case MSG_LIST_GAMES: {
                std::cout << "[LobbyServer] Client '" << username << "' requested games list"
                          << std::endl;
                send_games_list(client_socket);
                break;
            }

            case MSG_CREATE_GAME: {
                std::string game_name = read_string(client_socket);
                uint8_t max_players;
                client_socket.recvall(&max_players, sizeof(max_players));

                std::cout << "[LobbyServer] Client '" << username << "' creating game: " << game_name
                          << std::endl;

                uint16_t game_id = lobby_manager.create_game(game_name, username, max_players);

                if (game_id == 0) {
                    send_error(client_socket, ERR_ALREADY_IN_GAME, "You are already in a game");
                } else {
                    send_game_created(client_socket, game_id);
                }
                break;
            }

            case MSG_JOIN_GAME: {
                uint16_t game_id = read_uint16(client_socket);

                std::cout << "[LobbyServer] Client '" << username << "' joining game: " << game_id
                          << std::endl;

                if (!lobby_manager.join_game(game_id, username)) {
                    send_error(client_socket, ERR_GAME_FULL, "Game is full or already started");
                } else {
                    send_game_joined(client_socket, game_id);
                }
                break;
            }

            case MSG_LEAVE_GAME: {
                uint16_t game_id = read_uint16(client_socket);

                std::cout << "[LobbyServer] Client '" << username
                          << "' requested to leave game: " << game_id << std::endl;

                if (!lobby_manager.is_player_in_game(username)) {
                    send_error(client_socket, ERR_PLAYER_NOT_IN_GAME, "You are not in any game");
                    break;
                }

                if (lobby_manager.get_player_game(username) != game_id) {
                    send_error(client_socket, ERR_PLAYER_NOT_IN_GAME, "You are not in that game");
                    break;
                }

                lobby_manager.leave_game(username);
                std::cout << "[LobbyServer] Client '" << username << "' left game " << game_id
                          << std::endl;

                break;
            }

            default:
                std::cerr << "[LobbyServer] Unknown message type: " << static_cast<int>(msg_type)
                          << std::endl;
                client_connected = false;
                break;
            }
        }

    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        if (error_msg.find("Connection closed") != std::string::npos ||
            error_msg.find("EOF") != std::string::npos) {
            std::cout << "[LobbyServer] Client '" << username << "' disconnected: Connection closed"
                      << std::endl;
        } else {
            std::cerr << "[LobbyServer] Error handling client '" << username << "': " << e.what()
                      << std::endl;
        }
    }

    try {
        if (!username.empty() && lobby_manager.is_player_in_game(username)) {
            uint16_t game_id = lobby_manager.get_player_game(username);
            std::cout << "[LobbyServer] Cleaning up player '" << username << "' from game "
                      << game_id << std::endl;
            lobby_manager.leave_game(username);
        }
    } catch (const std::exception& e) {
        std::cerr << "[LobbyServer] Error during cleanup for '" << username << "': " << e.what()
                  << std::endl;
    }
}

std::string LobbyServer::receive_username(Socket& client_socket) {
    uint8_t msg_type = read_message_type(client_socket);

    if (msg_type != MSG_USERNAME) {
        throw std::runtime_error("Expected USERNAME message");
    }

    std::cout << "[LobbyServer] DEBUG: Reading username string..." << std::endl;
    return read_string(client_socket);
}

void LobbyServer::send_welcome(Socket& client_socket, const std::string& username) {
    std::cout << "[LobbyServer] DEBUG: Sending welcome message..." << std::endl;
    std::string message = "Welcome to Need for Speed 2D, " + username + "!";
    auto buffer = LobbyProtocol::serialize_welcome(message);
    client_socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyServer] DEBUG: Welcome sent!" << std::endl;
}

void LobbyServer::send_games_list(Socket& client_socket) {
    std::vector<GameInfo> games;

    for (const auto& pair : lobby_manager.get_all_games()) {
        const GameRoom& room = *pair.second;

        GameInfo info;
        info.game_id = room.get_game_id();

        std::string name = room.get_game_name();
        size_t copy_len = std::min(name.length(), sizeof(info.game_name) - 1);
        std::memcpy(info.game_name, name.c_str(), copy_len);
        info.game_name[copy_len] = '\0';

        info.current_players = room.get_player_count();
        info.max_players = room.get_max_players();
        info.is_started = room.is_started();

        games.push_back(info);
    }

    auto buffer = LobbyProtocol::serialize_games_list(games);
    client_socket.sendall(buffer.data(), buffer.size());
}

void LobbyServer::send_game_created(Socket& client_socket, uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_game_created(game_id);
    client_socket.sendall(buffer.data(), buffer.size());
}

void LobbyServer::send_game_joined(Socket& client_socket, uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_game_joined(game_id);
    client_socket.sendall(buffer.data(), buffer.size());
}

void LobbyServer::send_error(Socket& client_socket, LobbyErrorCode error_code,
                             const std::string& message) {
    auto buffer = LobbyProtocol::serialize_error(error_code, message);
    client_socket.sendall(buffer.data(), buffer.size());
}

// uint8_t LobbyServer::read_message_type(Socket& client_socket) {
//     uint8_t type;
//     int bytes = client_socket.recvall(&type, sizeof(type));
//     if (bytes == 0) {
//         throw std::runtime_error("Connection closed by client");
//     }
//     return type;
// }

uint8_t LobbyServer::read_message_type(Socket& client_socket) {
    uint8_t type;
    int bytes = client_socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        throw std::runtime_error("Connection closed by client");
    }
    return type;
}

std::string LobbyServer::read_string(Socket& client_socket) {
    std::cout << "[LobbyServer] DEBUG: read_string() - Reading length..." << std::endl;

    uint16_t len_net;
    int bytes = client_socket.recvall(&len_net, sizeof(len_net));

    std::cout << "[LobbyServer] DEBUG: read_string() - Bytes read: " << bytes << std::endl;

    if (bytes == 0) {
        throw std::runtime_error("Connection closed while reading string length");
    }

    uint16_t len = ntohs(len_net);
    std::cout << "[LobbyServer] DEBUG: read_string() - String length (after ntohs): " << len
              << std::endl;

    std::vector<char> buffer(len);
    client_socket.recvall(buffer.data(), len);

    std::string result(buffer.begin(), buffer.end());
    std::cout << "[LobbyServer] DEBUG: read_string() - String received: " << result << std::endl;

    return result;
}

uint16_t LobbyServer::read_uint16(Socket& client_socket) {
    uint16_t value_net;
    client_socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void LobbyServer::stop() {
    running = false;
}

LobbyServer::~LobbyServer() {
    stop();
}
