#include "server_protocol.h"

#include <iostream>
#include <stdexcept>
#include <netinet/in.h>

#include "../common_src/dtos.h"
#include "common_src/lobby_protocol.h"

ServerProtocol::ServerProtocol(Socket &skt, LobbyManager& manager) : socket(skt), lobby_manager(manager) {}


uint8_t ServerProtocol::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        throw std::runtime_error("Connection closed");
    }
    return type;
}

std::string ServerProtocol::read_string() {
    std::cout << "[LobbyServer] DEBUG: read_string() - Reading length..." << std::endl;

    uint16_t len_net;
    int bytes_read = socket.recvall(&len_net, sizeof(len_net));
    std::cout << "[LobbyServer] DEBUG: read_string() - Bytes read: "
              << bytes_read << std::endl;

    if (bytes_read == 0) {
        throw std::runtime_error("Connection closed while reading string length");
    }

    // CRÍTICO: Convertir de network byte order a host byte order
    uint16_t len = ntohs(len_net);

    std::cout << "[LobbyServer] DEBUG: read_string() - String length (after ntohs): "
              << len << std::endl;

    // Validación de sanidad
    if (len == 0 || len > 1024) {
        throw std::runtime_error("Invalid string length: " + std::to_string(len));
    }

    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);

    std::cout << "[LobbyServer] DEBUG: read_string() - String received: "
              << std::string(buffer.begin(), buffer.end()) << std::endl;

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

void ServerProtocol::process_client_messages(const std::string& username) {
    try {
            uint8_t msg_type = read_message_type();

            switch (msg_type) {
                case MSG_LIST_GAMES: {
                    std::cout << "[LobbyServer] Client '" << username << "' requested games list" << std::endl;

                    auto games = lobby_manager.get_games_list();
                    auto response = LobbyProtocol::serialize_games_list(games);
                    send_buffer(response);
                    break;
                }

                case MSG_CREATE_GAME: {
                    std::string game_name = read_string();
                    uint8_t max_players;
                    socket.recvall(&max_players, sizeof(max_players));

                    std::cout << "[LobbyServer] Client '" << username
                              << "' creating game: " << game_name << std::endl;

                    uint16_t game_id = lobby_manager.create_game(game_name, username, max_players);

                    if (game_id == 0) {
                        // El jugador ya está en una partida
                        auto response = LobbyProtocol::serialize_error(
                            ERR_ALREADY_IN_GAME, "You are already in a game");
                        send_buffer(response);
                    } else {
                        auto response = LobbyProtocol::serialize_game_created(game_id);
                        send_buffer(response);
                    }
                    break;
                }

                case MSG_JOIN_GAME: {
                    uint16_t game_id = read_uint16();


                    std::cout << "[LobbyServer] Client '" << username
                              << "' joining game: " << game_id << std::endl;

                    bool success = lobby_manager.join_game(game_id, username);

                    if (success) {
                        auto response = LobbyProtocol::serialize_game_joined(game_id);
                        send_buffer(response);
                    } else {
                        // Determinar el motivo del error
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

                case MSG_LEAVE_GAME: {
                    std::cout << "[LobbyServer] Client '" << username
                              << "' leaving game" << std::endl;

                    bool success = lobby_manager.leave_game(username);

                    if (success) {
                        // Enviar confirmación (puedes crear un nuevo mensaje MSG_GAME_LEFT)
                        std::cout << "[LobbyServer] Client '" << username
                                  << "' left the game successfully" << std::endl;
                    }
                    break;
                }

                case MSG_START_GAME: {
                    uint16_t game_id = read_uint16();

                    std::cout << "[LobbyServer] Client '" << username
                              << "' trying to start game " << game_id << std::endl;

                    bool success = lobby_manager.start_game(game_id, username);

                    if (success) {
                        auto response = LobbyProtocol::serialize_game_started(game_id);
                        send_buffer(response);
                        // TODO: Aquí irá la transición a la fase de juego
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
    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Client '" << username
                  << "' disconnected: " << e.what() << std::endl;

        // Limpiar: remover al jugador de su partida
        lobby_manager.leave_game(username);
    }
}

bool ServerProtocol::handle_client() {
    try {
        std::cout << "[LobbyServer] DEBUG: Starting to handle new client" << std::endl;

        uint8_t msg_type = read_message_type();
        if (msg_type != MSG_USERNAME) {
            std::cout << "[LobbyServer] Expected USERNAME message, got: "
                      << static_cast<int>(msg_type) << std::endl;
            return false; // error de protocolo
        }

        std::string username = read_string();
        std::cout << "[LobbyServer] Client connected: " << username << std::endl;

        std::string welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        auto welcome_buffer = LobbyProtocol::serialize_welcome(welcome_msg);
        send_buffer(welcome_buffer);

        // Procesar mensajes del cliente
        process_client_messages(username);

        return true; // conexión sigue activa

    } catch (const std::exception& e) {
        std::cout << "[LobbyServer] Error handling client: " << e.what() << std::endl;
        return false; // conexión cerrada o error fatal
    }
}

