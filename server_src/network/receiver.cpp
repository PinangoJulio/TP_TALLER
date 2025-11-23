#include "receiver.h"

#include <sys/socket.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#define RUTA_MAPS "server_src/city_maps/"

Receiver::Receiver(ServerProtocol& protocol, int id, Queue<GameState>& sender_messages_queue,
                   std::atomic<bool>& is_running, MatchesMonitor& monitor)
    : protocol(protocol), id(id), match_id(-1), sender_messages_queue(sender_messages_queue),
      is_running(is_running), monitor(monitor), commands_queue(),
      sender(protocol, sender_messages_queue, is_running, id) {}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
Receiver::get_city_maps() {
    namespace fs = std::filesystem;
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
        ciudades_y_carreras;

    for (const auto& entry : fs::directory_iterator(RUTA_MAPS)) {
        if (!entry.is_directory())
            continue;

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
            std::cout << "[Receiver] Ciudad: " << nombre_ciudad
                      << ", carreras encontradas: " << carreras.size() << "\n";
        }
    }
    return ciudades_y_carreras;
}

void Receiver::handle_lobby() {
    int current_match_id = -1;
    try {
        uint8_t msg_type_user = protocol.read_message_type();
        if (msg_type_user != MSG_USERNAME) {
            std::cerr << "[Receiver] Invalid protocol start (expected MSG_USERNAME)\n";
            return;
        }

        username = protocol.read_string();
        std::cout << "[Receiver] Player connected: " << username << std::endl;

        auto welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        protocol.send_buffer(LobbyProtocol::serialize_welcome(welcome_msg));

        // --- Bucle principal del lobby ---
        bool in_lobby = true;

        while (is_running && in_lobby) {
            uint8_t msg_type = protocol.read_message_type();

            switch (msg_type) {
            // ------------------------------------------------------------
            case MSG_LIST_GAMES: {
                std::cout << "[Receiver] " << username << " requested games list\n";

                std::vector<GameInfo> games = monitor.list_available_matches();

                std::cout << "[Receiver] Sending " << games.size() << " games to " << username
                          << "\n";

                auto response = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(response);
                break;
            }
            // ------------------------------------------------------------
            case MSG_CREATE_GAME: {
                std::string game_name = protocol.read_string();
                uint8_t max_players = protocol.get_uint8_t();
                uint8_t num_races = protocol.get_uint8_t();

                std::cout << "[Receiver] " << username << " creating game: " << game_name
                          << " (max: " << (int)max_players << ", races: " << (int)num_races << ")\n";

                if (current_match_id != -1) {
                    std::cout << "[Receiver] ERROR: " << username << " is already in match "
                              << current_match_id << "\n";
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game"));
                    break;
                }

                if (monitor.is_player_in_match(username)) {
                    std::cout << "[Receiver] ERROR: Monitor says " << username
                              << " is already in a match!\n";
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game (monitor check)"));
                    break;
                }

                // Crear la partida (el host se agrega automáticamente)
                int match_id =
                    monitor.create_match(max_players, username, id, sender_messages_queue);
                if (match_id < 0) {  // ✅ CORRECTO: create_match retorna -1 en error
                    protocol.send_buffer(
                        LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Error creating match"));
                    break;
                }

                current_match_id = match_id;
                this->match_id = match_id;

                // Registrar socket
                monitor.register_player_socket(match_id, username, protocol.get_socket());

                // Recibir selección de carreras
                std::vector<RaceConfig> races;
                for (int i = 0; i < num_races; ++i) {
                    std::string city = protocol.read_string();
                    std::string map = protocol.read_string();
                    races.push_back({city, map});
                    std::cout << "[Receiver]   Race " << (i + 1) << ": " << city << " - " << map
                              << "\n";
                }

                // ✅ GUARDAR LAS CARRERAS
                monitor.add_races_to_match(match_id, races);

                protocol.send_buffer(LobbyProtocol::serialize_game_created(match_id));
                std::cout << "[Receiver] Game created with ID: " << match_id << "\n";
                break;
            }
            // ------------------------------------------------------------
            case MSG_JOIN_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());
                std::cout << "[Receiver] " << username << " joining game " << game_id << "\n";

                if (current_match_id != -1) {
                    std::cout << "[Receiver] ERROR: " << username << " is already in match "
                              << current_match_id << "\n";
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game"));
                    break;
                }

                // 1. CAPTURAR SNAPSHOT **ANTES** DE AGREGAR AL JUGADOR
                auto existing_players = monitor.get_match_players_snapshot(game_id);

                // 2. REGISTRAR SOCKET **ANTES** DE JOIN
                monitor.register_player_socket(game_id, username, protocol.get_socket());

                // 3. HACER JOIN
                bool success = monitor.join_match(game_id, username, id, sender_messages_queue);

                if (!success) {
                    monitor.unregister_player_socket(game_id, username);
                    protocol.send_buffer(
                        LobbyProtocol::serialize_error(ERR_GAME_FULL, "Game is full or started"));
                    break;
                }

                current_match_id = game_id;
                this->match_id = game_id;

                // 4. ENVIAR CONFIRMACIÓN AL NUEVO JUGADOR
                protocol.send_buffer(
                    LobbyProtocol::serialize_game_joined(static_cast<uint16_t>(game_id)));
                std::cout << "[Receiver] " << username << " joined match " << game_id << std::endl;

                // 5. ENVIAR SNAPSHOT DE JUGADORES EXISTENTES
                std::cout << "[Receiver] Sending room snapshot to " << username << " ("
                          << existing_players.size() << " existing players)" << std::endl;

                for (const auto& [player_id, player_info] : existing_players) {
                    // Notificar que este jugador existe
                    auto joined_notif =
                        LobbyProtocol::serialize_player_joined_notification(player_info.name);
                    protocol.send_buffer(joined_notif);

                    std::cout << "[Receiver]   → Player exists: " << player_info.name << std::endl;

                    // Si tiene auto seleccionado, notificarlo
                    if (!player_info.car_name.empty()) {
                        auto car_notif = LobbyProtocol::serialize_car_selected_notification(
                            player_info.name, player_info.car_name, player_info.car_type);
                        protocol.send_buffer(car_notif);

                        std::cout << "[Receiver]   → Car: " << player_info.car_name << std::endl;
                    }

                    // Si está ready, notificarlo
                    if (player_info.is_ready) {
                        auto ready_notif = LobbyProtocol::serialize_player_ready_notification(
                            player_info.name, true);
                        protocol.send_buffer(ready_notif);

                        std::cout << "[Receiver]   → Ready: YES" << std::endl;
                    }
                }

                // Enviar marcador de fin de snapshot
                std::vector<uint8_t> end_marker;
                end_marker.push_back(MSG_ROOM_SNAPSHOT);
                end_marker.push_back(0);
                end_marker.push_back(0);
                protocol.send_buffer(end_marker);

                std::cout << "[Receiver] ✅ Snapshot sent with END marker to " << username
                          << std::endl;

                // 6. BROADCAST A LOS DEMÁS **DESPUÉS**
                auto joined_notif = LobbyProtocol::serialize_player_joined_notification(username);
                monitor.broadcast_to_match(game_id, joined_notif, username);

                std::cout << "[Receiver] Broadcasted join notification (excluding " << username
                          << ")" << std::endl;

                break;
            }
            // ------------------------------------------------------------
            case MSG_SELECT_CAR: {
                std::string car_name = protocol.read_string();
                std::string car_type = protocol.read_string();

                std::cout << "[Receiver] " << username << " selected car: " << car_name << " ("
                          << car_type << ")\n";

                if (current_match_id == -1) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME,
                                                                        "You are not in any game"));
                    break;
                }

                // Guardar el auto
                if (!monitor.set_player_car(username, car_name, car_type)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_INVALID_CAR_INDEX,
                                                                        "Failed to select car"));
                    break;
                }

                // Enviar ACK al cliente
                protocol.send_buffer(LobbyProtocol::serialize_car_selected_ack(car_name, car_type));
                std::cout << "[Receiver] ✅ ACK sent to " << username << std::endl;

                // Broadcast a TODOS EXCEPTO al que seleccionó
                auto notif =
                    LobbyProtocol::serialize_car_selected_notification(username, car_name, car_type);
                monitor.broadcast_to_match(current_match_id, notif, username);
                std::cout << "[Receiver] ✅ Broadcast triggered (excluding " << username << ")"
                          << std::endl;

                break;
            }
            // ------------------------------------------------------------
            case MSG_LEAVE_GAME: {
                uint16_t game_id = protocol.read_uint16();

                std::cout << "[Receiver] " << username << " leaving game " << game_id << "\n";

                // Validar que esté en esa partida
                if (current_match_id != game_id) {
                    std::cout << "[Receiver] ERROR: " << username << " is not in game " << game_id
                              << " (current: " << current_match_id << ")\n";
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME,
                                                                        "You are not in that game"));
                    break;
                }

                // Eliminar del monitor
                monitor.leave_match(username);

                current_match_id = -1;
                this->match_id = -1;

                std::cout << "[Receiver] " << username << " successfully left game " << game_id
                          << std::endl;

                // Enviar lista de partidas actualizada
                std::vector<GameInfo> games = monitor.list_available_matches();
                auto buffer = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(buffer);

                std::cout << "[Receiver] Sent updated games list (" << games.size() << " games)"
                          << std::endl;

                break;
            }
            // ------------------------------------------------------------
            case MSG_PLAYER_READY: {
                uint8_t is_ready = protocol.get_uint8_t();

                std::cout << "[Receiver] " << username << " set ready: " << (is_ready ? "YES" : "NO")
                          << "\n";

                if (current_match_id == -1) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME,
                                                                        "You are not in any game"));
                    break;
                }

                if (!monitor.set_player_ready(username, is_ready != 0)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_INVALID_CAR_INDEX, "You must select a car before being ready"));
                    break;
                }

                auto notif =
                    LobbyProtocol::serialize_player_ready_notification(username, is_ready != 0);
                monitor.broadcast_to_match(current_match_id, notif, username);

                break;
            }
            // ------------------------------------------------------------
            case MSG_START_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());

                std::cout << "[Receiver] " << username << " starting game " << game_id << "\n";

                if (current_match_id != game_id) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME,
                                                                        "You are not in this game"));
                    break;
                }

                // Validar que se pueda iniciar
                if (!monitor.is_match_ready(game_id)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_PLAYERS_NOT_READY, "Not all players are ready or no races configured"));
                    break;
                }

                // ✅ INICIAR LA PARTIDA (arranca el GameLoop)
                monitor.start_match(game_id);

                // Broadcast MSG_GAME_STARTED a todos
                auto buffer = LobbyProtocol::serialize_game_started(static_cast<uint16_t>(game_id));
                monitor.broadcast_to_match(game_id, buffer, "");

                std::cout << "[Receiver] ✅ Game " << game_id << " started successfully!"
                          << std::endl;

                // ✅ TRANSICIÓN AL JUEGO
                in_lobby = false;

                break;
            }
            // ------------------------------------------------------------
            default:
                std::cerr << "[Receiver] Unknown message type: " << static_cast<int>(msg_type)
                          << std::endl;
                in_lobby = false;
                break;
            }
        }

        // FIN DEL LOBBY - handle_match_messages() se llamará desde run()

    } catch (const std::exception& e) {
        std::string error_msg = e.what();

        if (error_msg.find("Connection closed") != std::string::npos) {
            std::cout << "[Receiver] Player " << username << " disconnected" << std::endl;
        } else {
            std::cerr << "[Receiver] Lobby error: " << error_msg << std::endl;
        }

        // Cleanup en caso de desconexión
        if (!username.empty() && current_match_id != -1) {
            std::cout << "[Receiver] Cleaning up " << username << " from match " << current_match_id
                      << " (disconnect)" << std::endl;

            try {
                monitor.leave_match(username);
                std::cout << "[Receiver] ✅ " << username << " cleaned up successfully" << std::endl;
            } catch (const std::exception& cleanup_error) {
                std::cerr << "[Receiver] ❌ Failed to cleanup: " << cleanup_error.what()
                          << std::endl;
            }
        }

        is_running = false;
    }
}

void Receiver::handle_match_messages() {
    std::cout << "[Receiver]  Game loop started - listening for player commands..." << std::endl;

    bool disconnected = false;

    try {
        // ♾- Espera comandos del jugador constantemente
        while (is_running) {
            ComandMatchDTO comand_match;
            comand_match.player_id = id;

            try {
                // bloquea aca hasta recibir un comando del cliente
                protocol.read_command_client(comand_match);
            } catch (...) {
                // Conexión cerrada o error de lectura
                break;
            }

            try {
                // pushear a la queue (GameLoop lo consumirá)
                commands_queue->try_push(comand_match);

                if (disconnected)
                    break;

                if (comand_match.command == GameCommand::DISCONNECT) {
                    disconnected = true;
                    std::cout << "[Receiver]Player " << username << " sent DISCONNECT command"
                              << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[Receiver] Error pushing command: " << e.what() << std::endl;
                break;
            }
        }
        is_running = false;
    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Match error: " << e.what() << std::endl;
    }

    std::cout << "[Receiver] Player " << username << " exited match " << match_id << std::endl;
}

void Receiver::run() {
    //  FASE LOBBY
    handle_lobby();

    // VERIFICAR SI PASÓ A FASE DE JUEGO
    if (match_id != -1 && is_running) {
        std::cout << "[Receiver] Player " << username << " transitioning to GAME MODE for match "
                  << match_id << std::endl;
        // OBTENER QUEUE DE COMANDOS DEL MATCH
        commands_queue = monitor.get_command_queue(match_id);

        // INICIAR SENDER (para enviar GameState a este jugador)
        sender.start();
        std::cout << "[Receiver] Sender started for player " << username << std::endl;

        // ⃣FASE JUEGO
        handle_match_messages();
    }

    if (match_id != -1) {
        monitor.delete_player_from_match(id, match_id);
    }
    sender_messages_queue.close();
    if (match_id != -1) {
        sender.join();
    }

    std::cout << "[Receiver] Player " << username << " fully disconnected" << std::endl;
}

void Receiver::kill() {
    is_running = false;
}

bool Receiver::status() {
    return is_running;
}

Receiver::~Receiver() {}
