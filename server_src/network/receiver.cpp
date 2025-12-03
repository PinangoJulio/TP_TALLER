#include "receiver.h"

#include <sys/socket.h>

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <arpa/inet.h>

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

        auto welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        protocol.send_buffer(LobbyProtocol::serialize_welcome(welcome_msg));

        bool in_lobby = true;

        while (is_running && in_lobby) {
            
            if (!is_running) {
                throw std::runtime_error("Server shutdown");
            }
            
            uint8_t msg_type = protocol.read_message_type();
            switch (msg_type) {
            // ------------------------------------------------------------
            case MSG_LIST_GAMES: {
                std::cout << "[Receiver] " << username << " requested games list\n";

                std::vector<GameInfo> games = monitor.list_available_matches();


                auto response = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(response);
                break;
            }
            // ------------------------------------------------------------
            case MSG_CREATE_GAME: {
                std::string game_name = protocol.read_string();
                uint8_t max_players = protocol.get_uint8_t();
                uint8_t num_races = protocol.get_uint8_t();

                if (current_match_id != -1) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game"));
                    break;
                }

                if (monitor.is_player_in_match(username)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(
                        ERR_ALREADY_IN_GAME, "You are already in a game (monitor check)"));
                    break;
                }

                // Crear la partida (el host se agrega automáticamente)
                int new_match_id =
                    monitor.create_match(max_players, username, id, sender_messages_queue);
                if (new_match_id < 0) {  
                    protocol.send_buffer(
                        LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Error creating match"));
                    break;
                }

                current_match_id = new_match_id;
                this->match_id = new_match_id;

                // Registrar socket
                monitor.register_player_socket(match_id, username, protocol.get_socket());

                // Recibir selección de carreras
                std::vector<ServerRaceConfig> races;
                for (int i = 0; i < num_races; ++i) {
                    std::string city = protocol.read_string();
                    std::string map = protocol.read_string();
                    races.push_back({city, map});

                }

                
                monitor.add_races_to_match(match_id, races);

                protocol.send_buffer(LobbyProtocol::serialize_game_created(match_id));

                // ENVIAR YAML AL CLIENTE
                std::vector<std::string> yaml_paths = monitor.get_race_paths(match_id);
                if (!yaml_paths.empty()) {
                    protocol.send_race_paths(yaml_paths);

                }

                break;
            }
            // ------------------------------------------------------------
            case MSG_JOIN_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());

                if (current_match_id != -1) {

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

                // 5. ENVIAR SNAPSHOT DE JUGADORES EXISTENTES

                for (const auto& [player_id, player_info] : existing_players) {
                    // Notificar que este jugador existe
                    auto joined_notif =
                        LobbyProtocol::serialize_player_joined_notification(player_info.name);
                    protocol.send_buffer(joined_notif);


                    // Si tiene auto seleccionado, notificarlo
                    if (!player_info.car_name.empty()) {
                        auto car_notif = LobbyProtocol::serialize_car_selected_notification(
                            player_info.name, player_info.car_name, player_info.car_type);
                        protocol.send_buffer(car_notif);

                    }

                    // Si está ready, notificarlo
                    if (player_info.is_ready) {
                        auto ready_notif = LobbyProtocol::serialize_player_ready_notification(
                            player_info.name, true);
                        protocol.send_buffer(ready_notif);

                    }
                }

                // Enviar marcador de fin de snapshot
                std::vector<uint8_t> end_marker;
                end_marker.push_back(MSG_ROOM_SNAPSHOT);
                end_marker.push_back(0);
                end_marker.push_back(0);
                protocol.send_buffer(end_marker);



                // ENVIAR YAML AL CLIENTE
                std::vector<std::string> yaml_paths = monitor.get_race_paths(game_id);
                if (!yaml_paths.empty()) {
                    protocol.send_race_paths(yaml_paths);

                }

                // 6. BROADCAST A LOS DEMÁS **DESPUÉS**
                auto joined_notif = LobbyProtocol::serialize_player_joined_notification(username);
                monitor.broadcast_to_match(game_id, joined_notif, username);



                break;
            }
            // ------------------------------------------------------------
            case MSG_SELECT_CAR: {
                std::string car_name = protocol.read_string();
                std::string car_type = protocol.read_string();



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

                // Broadcast a TODOS EXCEPTO al que seleccionó
                auto notif =
                    LobbyProtocol::serialize_car_selected_notification(username, car_name, car_type);
                monitor.broadcast_to_match(current_match_id, notif, username);


                break;
            }
            // ------------------------------------------------------------
            case MSG_LEAVE_GAME: {
                uint16_t game_id = protocol.read_uint16();


                if (current_match_id != game_id) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME,
                                                                        "You are not in that game"));
                    break;
                }

                
                auto left_notif = LobbyProtocol::serialize_player_left_notification(username);
                monitor.broadcast_to_match(game_id, left_notif, username);

                // Eliminar del monitor
                monitor.leave_match(username);

                current_match_id = -1;
                this->match_id = -1;


                // Enviar lista de partidas actualizada
                std::vector<GameInfo> games = monitor.list_available_matches();
                auto buffer = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(buffer);


                break;
            }
            // ------------------------------------------------------------
            case MSG_PLAYER_READY: {
                uint8_t is_ready = protocol.get_uint8_t();

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

                
                // Durante el juego, el Sender maneja toda la comunicación
                if (current_match_id != -1) {
                    auto notif =
                        LobbyProtocol::serialize_player_ready_notification(username, is_ready != 0);
                    monitor.broadcast_to_match(current_match_id, notif, username);
                }

                break;
            }
            // ------------------------------------------------------------
            case MSG_START_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());

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

                
                monitor.start_match(game_id);

                std::vector<uint8_t> start_msg = {
                    static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED)};

                // Enviar al jugador que solicitó el inicio
                protocol.send_buffer(start_msg);

                // Broadcast a todos los demás
                monitor.broadcast_to_match(game_id, start_msg, username);
                
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

        // OBTENER QUEUE DE COMANDOS DEL MATCH
        commands_queue = monitor.get_command_queue(match_id);

        // INICIAR SENDER (para enviar GameState a este jugador)
        sender.start();

        
        // porque el hilo Sender (ClientMonitor) es el único que debe escribir durante la partida.
        // Eliminamos el socket del registro de MatchesMonitor para este jugador.
        try {
            monitor.unregister_player_socket(match_id, username);
        } catch (const std::exception& e) {
            std::cerr << "[Receiver] Warning: could not unregister socket for " << username
                      << ": " << e.what() << std::endl;
        }
    } catch (const std::exception& e) {
        std::string error_msg = e.what();

        
        if (error_msg.find("Server shutdown") != std::string::npos) {
            std::cout << "[Receiver " << username << "] Server is shutting down" << std::endl;
            
            
            try {
                std::vector<uint8_t> shutdown_msg;
                shutdown_msg.push_back(MSG_ERROR);
                shutdown_msg.push_back(0xFF); // Código especial
                std::string msg = "SERVER SHUTDOWN - DISCONNECTING";
                uint16_t len = htons(msg.size());
                shutdown_msg.push_back(reinterpret_cast<uint8_t*>(&len)[0]);
                shutdown_msg.push_back(reinterpret_cast<uint8_t*>(&len)[1]);
                shutdown_msg.insert(shutdown_msg.end(), msg.begin(), msg.end());
                
                protocol.send_buffer(shutdown_msg);
            } catch (...) {
                // Ignorar errores al enviar
            }
        }

        if (error_msg.find("Connection closed") != std::string::npos) {
            std::cout << "[Receiver] Player " << username << " disconnected" << std::endl;
        } else {
            std::cerr << "[Receiver] Lobby error: " << error_msg << std::endl;
        }

        // Cleanup en caso de desconexión
        if (!username.empty() && current_match_id != -1) {

            try {
                monitor.leave_match(username);
                std::cout << "[Receiver]   " << username << " cleaned up successfully" << std::endl;
            } catch (const std::exception& cleanup_error) {
                std::cerr << "[Receiver]   Failed to cleanup: " << cleanup_error.what()
                          << std::endl;
            }
        }
        is_running = false;
    }
}

void Receiver::handle_match_messages() {

    try {
        while (is_running) {
            if (!is_running) {
                break;
            }
            
            ComandMatchDTO comand_match;
            comand_match.player_id = id;

            try {
                protocol.read_command_client(comand_match);
            } catch (const std::exception& e) {
                // Socket cerrado o error de lectura
                std::string error_msg = e.what();
                if (error_msg.find("shutdown") != std::string::npos ||
                    error_msg.find("Connection closed") != std::string::npos ||
                    !is_running) {
                } else {
                    std::cerr << "[Receiver " << username << "] Read error: " << error_msg << std::endl;
                }
                break;
            }

            
            if (!is_running) {
                break;
            }

            try {
                commands_queue->try_push(comand_match);

                if (comand_match.command == GameCommand::DISCONNECT) {
                    break;
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
}

void Receiver::run() {
    //  FASE LOBBY
    handle_lobby();

    // VERIFICAR SI PASÓ A FASE DE JUEGO
    handle_match_messages();

    
    // (evita acceso a memoria liberada durante shutdown)
    if (match_id != -1 && is_running) {
        try {
            monitor.delete_player_from_match(id, match_id);
        } catch (const std::exception& e) {
            std::cerr << "[Receiver] Warning: Could not delete player from match: "
                      << e.what() << std::endl;
        }
    }

    sender_messages_queue.close();
    sender.join();

}

void Receiver::kill() {
    is_running = false;
}

bool Receiver::status() {
    return is_running;
}

Receiver::~Receiver() {
    sender.join();
}
