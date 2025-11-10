#include "receiver.h"
#include <filesystem>
#include <iostream>
#include <sys/socket.h>

#define RUTA_MAPS "server_src/city_maps/"

Receiver::Receiver(ServerProtocol& protocol, int id, Queue<GameState>& sender_messages_queue, std::atomic<bool>& is_running, MatchesMonitor& monitor, LobbyManager& lobby_manager)
    : protocol(protocol), id(id), match_id(-1), sender_messages_queue(sender_messages_queue),
      is_running(is_running), monitor(monitor),
      commands_queue(), sender(protocol, sender_messages_queue, is_running, id),
      lobby_manager(lobby_manager){}


// ------------------------------------------------------
// Devuelve vector de (ciudad, [(yaml, png)])
// ------------------------------------------------------
std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> Receiver::get_city_maps() {
    namespace fs = std::filesystem;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> ciudades_y_carreras;

    for (const auto& entry : fs::directory_iterator(RUTA_MAPS)) {
        if (!entry.is_directory()) continue;

        std::string nombre_ciudad = entry.path().filename().string();
        std::vector<std::pair<std::string, std::string>> carreras;

        for (const auto& file : fs::directory_iterator(entry.path())) {
            auto path = file.path();
            if (path.extension() == ".yaml") {
                std::string base = path.stem().string();
                fs::path png_path = entry.path() / (base + ".png");
                if (fs::exists(png_path)) {
                    carreras.emplace_back(base + ".yaml", base + ".png");
                }
            }
        }

        if (!carreras.empty()) {
            ciudades_y_carreras.emplace_back(nombre_ciudad, carreras);
            std::cout << "Ciudad: " << nombre_ciudad << ", carreras encontradas: " << carreras.size() << "\n";
        }
    }
    return ciudades_y_carreras;
}


void Receiver::handle_lobby() {
    try {
        uint8_t msg_type_user = protocol.read_message_type();
        if (msg_type_user != MSG_USERNAME) {
            std::cerr << "[Lobby] Invalid protocol start (expected MSG_USERNAME)\n";
            return;
        }

        std::string username = protocol.read_string();
        std::cout << "[Lobby] Player connected: " << username << std::endl;

        auto welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        auto welcome_buffer = LobbyProtocol::serialize_welcome(welcome_msg);
        protocol.send_buffer(welcome_buffer);

        // --- Paso 2: bucle principal del lobby ---
        bool in_lobby = true;
        while (is_running && in_lobby) {
            uint8_t msg_type = protocol.read_message_type();

            switch (msg_type) {
                // ------------------------------------------------------------
                case MSG_LIST_GAMES: {
                    auto games = lobby_manager.get_games_list();
                    auto response = LobbyProtocol::serialize_games_list(games);
                    protocol.send_buffer(response);
                    break;
                }
                // ------------------------------------------------------------
                case MSG_CREATE_GAME: {
                    std::string game_name = protocol.read_string();
                    uint8_t max_players = protocol.get_max_amount_of_players();

                    uint16_t game_id = lobby_manager.create_game(game_name, username, max_players);
                    if (game_id == 0) {
                        auto response = LobbyProtocol::serialize_error(
                            ERR_ALREADY_IN_GAME, "You are already in a game");
                        protocol.send_buffer(response);
                    } else {
                        auto response = LobbyProtocol::serialize_game_created(game_id);
                        protocol.send_buffer(response);
                        std::cout << "[Lobby] Game created: " << game_name << " (" << game_id << ")\n";
                    }
                    break;
                }
                // ------------------------------------------------------------
                /*case MSG_LIST_CITY_MAPS: {
                    auto city_maps = get_city_maps();
                    auto response = LobbyProtocol::serialize_city_maps(city_maps);
                    protocol.send_buffer(response);
                    break;
                }*/
                // ------------------------------------------------------------
                case MSG_JOIN_GAME: {
                    uint16_t game_id = protocol.read_uint16();
                    bool success = lobby_manager.join_game(game_id, username);

                    if (success) {
                        auto response = LobbyProtocol::serialize_game_joined(game_id);
                        protocol.send_buffer(response);
                        std::cout << "[Lobby] " << username << " joined game " << game_id << std::endl;
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
                        protocol.send_buffer(response);
                    }
                    break;
                }

                // ------------------------------------------------------------
                case MSG_START_GAME: {
                    uint16_t game_id = protocol.read_uint16();
                    bool success = lobby_manager.start_game(game_id, username);

                    if (success) {
                        auto response = LobbyProtocol::serialize_game_started(game_id);
                        protocol.send_buffer(response);
                        std::cout << "[Lobby] Game " << game_id << " started by " << username << "\n";

                        // 🔹 Transición al modo juego
                        in_lobby = false;
                        // acá podrías guardar el match_id si querés
                        match_id = game_id;
                    } else {
                        auto response = LobbyProtocol::serialize_error(
                            ERR_NOT_HOST, "Only the host can start the game");
                        protocol.send_buffer(response);
                    }
                    break;
                }
                default:
                    std::cerr << "[Lobby] Unknown message type: " << static_cast<int>(msg_type) << std::endl;
                    break;
            }
        }

        if (!in_lobby) {
            std::cout << "[Lobby] Player " << username << " entering match " << match_id << std::endl;
            // podés iniciar handle_match_messages() o solo marcar transición
        }

    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Lobby error: " << e.what() << std::endl;
        is_running = false;
    }

    // Cuando termina el lobby, comienza la comunicación multihilo del juego
    // sender.start();
}


void Receiver::handle_match_messages() {
    //implementar comunicacion de juego
    is_running = false;
}


void Receiver::run() {
    handle_lobby();
    // handle_match_messages  --> llamar una vez que termina el lobby y arranca la comunicacion del juego
}

void Receiver::kill() { is_running = false; }

bool Receiver::status() { return is_running; }

Receiver::~Receiver() {}
