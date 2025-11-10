#include "server_protocol.h"
#include <iostream>
#include <stdexcept>
#include <netinet/in.h>
#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

ServerProtocol::ServerProtocol(Socket &skt, LobbyManager& manager)
    : socket(skt), lobby_manager(manager) {}

// --- Lectura básica de datos ---

uint8_t ServerProtocol::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0)
        throw std::runtime_error("Connection closed");
    return type;
}

std::string ServerProtocol::read_string() {
    uint16_t len_net;
    int bytes_read = socket.recvall(&len_net, sizeof(len_net));
    if (bytes_read == 0)
        throw std::runtime_error("Connection closed while reading string length");

    uint16_t len = ntohs(len_net);
    if (len == 0 || len > 1024)
        throw std::runtime_error("Invalid string length: " + std::to_string(len));

    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    return std::string(buffer.begin(), buffer.end());
}

uint16_t ServerProtocol::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void ServerProtocol::send_buffer(const std::vector<uint8_t>& buffer) {
    socket.sendall(buffer.data(), buffer.size());
}

// --- Manejo de mensajes del cliente ---
bool ServerProtocol::process_client_messages(const std::string& username) {
    try {
        uint8_t msg_type = read_message_type();

        switch (msg_type) {
            case MSG_LIST_GAMES: {
                auto games = lobby_manager.get_games_list();
                auto response = LobbyProtocol::serialize_games_list(games);
                send_buffer(response);
                break;
            }

            case MSG_CREATE_GAME: {
                std::string game_name = read_string();
                uint8_t max_players;
                socket.recvall(&max_players, sizeof(max_players));

                uint16_t game_id = lobby_manager.create_game(game_name, username, max_players);
                if (game_id == 0) {
                    auto response = LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game");
                    send_buffer(response);
                } else {
                    auto response = LobbyProtocol::serialize_game_created(game_id);
                    send_buffer(response);
                }
                break;
            }

            /*case MSG_LIST_CITY_MAPS: {  // 🆕 nuevo caso
                auto city_maps = lobby_manager.get_city_maps(); // obtiene {ciudad, png}
                auto response = LobbyProtocol::serialize_city_maps(city_maps);
                send_buffer(response);
                break;
            }*/

            case MSG_JOIN_GAME: {
                uint16_t game_id = read_uint16();
                bool success = lobby_manager.join_game(game_id, username);

                if (success) {
                    auto response = LobbyProtocol::serialize_game_joined(game_id);
                    send_buffer(response);
                } else {
                    std::string error_msg;
                    LobbyErrorCode error_code;

                    if (lobby_manager.is_player_in_game(username)) {
                        error_code = ERR_ALREADY_IN_GAME;
                        error_msg = "You are already in a game";
                    } else {
                        error_code = ERR_GAME_FULL;
                        error_msg = "Game is full or already started";
                    }

                    auto response = LobbyProtocol::serialize_error(error_code, error_msg);
                    send_buffer(response);
                }
                break;
            }
            case MSG_START_GAME: {
                uint16_t game_id = read_uint16();
                bool success = lobby_manager.start_game(game_id, username);
                if (success) {
                    auto response = LobbyProtocol::serialize_game_started(game_id);
                    send_buffer(response);
                    return false;
                } else {
                    auto response = LobbyProtocol::serialize_error(
                        ERR_NOT_HOST, "Only the host can start the game");
                    send_buffer(response);
                }
                break;
            }

            default:
                std::cout << "[LobbyServer] Unknown message type: "
                          << static_cast<int>(msg_type) << std::endl;
                break;
        }

        return true; // seguir procesando

    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Client '" << username
                  << "' disconnected: " << e.what() << std::endl;
        lobby_manager.leave_game(username);
        return false;
    }
}

// --- Manejo inicial de la conexión ---
bool ServerProtocol::handle_client() {
    try {
        uint8_t msg_type = read_message_type();
        if (msg_type != MSG_USERNAME)
            return false; // protocolo inválido

        std::string username = read_string();
        auto welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        auto welcome_buffer = LobbyProtocol::serialize_welcome(welcome_msg);
        send_buffer(welcome_buffer);

        while (process_client_messages(username))
            ; // loop hasta que el cliente empiece juego o se desconecte

        return true;

    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Error handling client: " << e.what() << std::endl;
        return false;
    }
}
