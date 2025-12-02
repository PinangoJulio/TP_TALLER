#include "receiver.h"

#include <sys/socket.h>
#include <cstring>
#include <iostream>
#include <vector>

Receiver::Receiver(ServerProtocol& protocol, int id, Queue<GameState>& sender_messages_queue,
                   std::atomic<bool>& is_running, MatchesMonitor& monitor)
    : protocol(protocol), id(id), match_id(-1), sender_messages_queue(sender_messages_queue),
      is_running(is_running), monitor(monitor), commands_queue(nullptr),
      sender(protocol, sender_messages_queue, is_running, id) {}

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
            case MSG_LIST_GAMES: {
                std::cout << "[Receiver] " << username << " requested games list\n";
                std::vector<GameInfo> games = monitor.list_available_matches();
                std::cout << "[Receiver] Sending " << games.size() << " games to " << username << "\n";
                auto response = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(response);
                break;
            }
            case MSG_CREATE_GAME: {
                std::string game_name = protocol.read_string();
                uint8_t max_players = protocol.get_uint8_t();
                uint8_t num_races = protocol.get_uint8_t();

                std::cout << "[Receiver] " << username << " creating game: " << game_name
                          << " (max: " << (int)max_players << ", races: " << (int)num_races << ")\n";

                if (current_match_id != -1) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Already in game"));
                    break;
                }

                int new_match_id = monitor.create_match(max_players, username, id, sender_messages_queue);
                if (new_match_id < 0) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Error creating match"));
                    break;
                }

                current_match_id = new_match_id;
                this->match_id = new_match_id;
                monitor.register_player_socket(match_id, username, protocol.get_socket());

                std::vector<ServerRaceConfig> races;
                for (int i = 0; i < num_races; ++i) {
                    std::string city = protocol.read_string();
                    std::string map = protocol.read_string();
                    races.push_back({city, map});
                    std::cout << "[Receiver]   Race " << (i + 1) << ": " << city << " - " << map << "\n";
                }
                monitor.add_races_to_match(match_id, races);

                protocol.send_buffer(LobbyProtocol::serialize_game_created(match_id));
                std::cout << "[Receiver] Game created with ID: " << match_id << "\n";

                std::vector<std::string> yaml_paths = monitor.get_race_paths(match_id);
                if (!yaml_paths.empty()) {
                    protocol.send_race_paths(yaml_paths);
                    std::cout << "[Receiver] Sent " << yaml_paths.size() << " race paths to " << username << std::endl;
                }
                break;
            }
            case MSG_JOIN_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());
                std::cout << "[Receiver] " << username << " joining game " << game_id << "\n";

                if (current_match_id != -1) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Already in game"));
                    break;
                }

                auto existing_players = monitor.get_match_players_snapshot(game_id);
                monitor.register_player_socket(game_id, username, protocol.get_socket());
                bool success = monitor.join_match(game_id, username, id, sender_messages_queue);

                if (!success) {
                    monitor.unregister_player_socket(game_id, username);
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_GAME_FULL, "Game full/started"));
                    break;
                }

                current_match_id = game_id;
                this->match_id = game_id;

                protocol.send_buffer(LobbyProtocol::serialize_game_joined(static_cast<uint16_t>(game_id)));
                std::cout << "[Receiver] " << username << " joined match " << game_id << std::endl;

                // Enviar Snapshot
                std::cout << "[Receiver] Sending room snapshot to " << username << " (" << existing_players.size() << " existing players)" << std::endl;
                for (const auto& [player_id, player_info] : existing_players) {
                    auto joined_notif = LobbyProtocol::serialize_player_joined_notification(player_info.name);
                    protocol.send_buffer(joined_notif);
                    std::cout << "[Receiver]   -> Player exists: " << player_info.name << std::endl;

                    if (!player_info.car_name.empty()) {
                        auto car_notif = LobbyProtocol::serialize_car_selected_notification(player_info.name, player_info.car_name, player_info.car_type);
                        protocol.send_buffer(car_notif);
                        std::cout << "[Receiver]   -> Car: " << player_info.car_name << std::endl;
                    }
                    if (player_info.is_ready) {
                        auto ready_notif = LobbyProtocol::serialize_player_ready_notification(player_info.name, true);
                        protocol.send_buffer(ready_notif);
                        std::cout << "[Receiver]   -> Ready: YES" << std::endl;
                    }
                }
                // Marcador fin snapshot
                std::vector<uint8_t> end_marker = {MSG_ROOM_SNAPSHOT, 0, 0};
                protocol.send_buffer(end_marker);
                std::cout << "[Receiver] ✅ Snapshot sent with END marker to " << username << std::endl;

                // Enviar Mapas
                std::vector<std::string> yaml_paths = monitor.get_race_paths(game_id);
                if (!yaml_paths.empty()) {
                    protocol.send_race_paths(yaml_paths);
                    std::cout << "[Receiver] Sent " << yaml_paths.size() << " race paths to " << username << std::endl;
                }

                auto joined_notif = LobbyProtocol::serialize_player_joined_notification(username);
                monitor.broadcast_to_match(game_id, joined_notif, username);
                std::cout << "[Receiver] Broadcasted join notification (excluding " << username << ")" << std::endl;
                break;
            }
            case MSG_SELECT_CAR: {
                std::string car_name = protocol.read_string();
                std::string car_type = protocol.read_string();
                std::cout << "[Receiver] " << username << " selected car: " << car_name << " (" << car_type << ")\n";

                if (current_match_id == -1) break;

                if (!monitor.set_player_car(username, car_name, car_type)) break;

                protocol.send_buffer(LobbyProtocol::serialize_car_selected_ack(car_name, car_type));
                std::cout << "[Receiver] ✅ ACK sent to " << username << std::endl;

                auto notif = LobbyProtocol::serialize_car_selected_notification(username, car_name, car_type);
                monitor.broadcast_to_match(current_match_id, notif, username);
                std::cout << "[Receiver] ✅ Broadcast triggered (excluding " << username << ")" << std::endl;
                break;
            }
            case MSG_LEAVE_GAME: {
                uint16_t game_id = protocol.read_uint16();
                std::cout << "[Receiver] " << username << " leaving game " << game_id << "\n";
                if (current_match_id != game_id) break;

                auto left_notif = LobbyProtocol::serialize_player_left_notification(username);
                monitor.broadcast_to_match(game_id, left_notif, username);
                monitor.leave_match(username);
                current_match_id = -1;
                this->match_id = -1;
                
                // Enviar lista actualizada
                std::vector<GameInfo> games = monitor.list_available_matches();
                auto buffer = LobbyProtocol::serialize_games_list(games);
                protocol.send_buffer(buffer);
                break;
            }
            case MSG_PLAYER_READY: {
                uint8_t is_ready = protocol.get_uint8_t();
                std::cout << "[Receiver] " << username << " set ready: " << (is_ready ? "YES" : "NO") << "\n";

                if (current_match_id == -1) break;

                monitor.set_player_ready(username, is_ready != 0);
                
                //para que sea automatica
                /*if (monitor.is_match_started(current_match_id)) {
                    std::cout << "[Receiver] 🚀 Partida iniciada (Auto-Start detectado). Saliendo del lobby.\n";
                    in_lobby = false; // Salir del bucle para ir a handle_match_messages
                }*/

                auto notif = LobbyProtocol::serialize_player_ready_notification(username, is_ready != 0);
                monitor.broadcast_to_match(current_match_id, notif, username);
                break;
            }
            case MSG_START_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());
                std::cout << "[Receiver] " << username << " starting game " << game_id << "\n";
                if (current_match_id != game_id) break;

                if (!monitor.is_match_ready(game_id)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYERS_NOT_READY, "Not all ready"));
                    break;
                }
                
                monitor.start_match(game_id);
                std::cout << "[Receiver] ✅ Game " << game_id << " started successfully!\n";
                in_lobby = false; // Salir del lobby
                break;
            }
            default:{
                //ESte if es necesario solo si queremos que la partida inicie con boton
                if (current_match_id != -1 && monitor.is_match_started(current_match_id)) {
                    std::cout << "[Receiver] 🚀 Detectado comando de juego (" << (int)msg_type 
                            << ") en Lobby. Transicionando a GameLoop.\n";

                }else{
                    //Si fuera autimatico dejamos solo esto
                    std::cerr << "[Receiver] Unknown message type: " << static_cast<int>(msg_type) << std::endl;
                    in_lobby = false; 
                }
                break;
            }
            }
        } 

        std::cout << "[Receiver] ✅ Exiting lobby loop for " << username
                  << " (match_id=" << match_id << ", is_running=" << is_running << ")" << std::endl;

        // OBTENER QUEUE DE COMANDOS DEL MATCH
        commands_queue = monitor.get_command_queue(match_id);

        // INICIAR SENDER (para enviar GameState a este jugador)
        sender.start();
        std::cout << "[Receiver] Sender started for player " << username << std::endl;

    } catch (const std::exception& e) {
        std::string error_msg = e.what();
        if (error_msg.find("Connection closed") != std::string::npos) {
            std::cout << "[Receiver] Player " << username << " disconnected" << std::endl;
        } else {
            std::cerr << "[Receiver] Lobby error: " << error_msg << std::endl;
        }
        if (!username.empty() && current_match_id != -1) {
            try { monitor.leave_match(username); } catch(...) {}
        }
        is_running = false;
    }
}

void Receiver::handle_match_messages() {
    std::cout << "[Receiver]  Game loop started - listening for player commands..." << std::endl;

    try {
        while (is_running) {
            ComandMatchDTO comand_match;
            comand_match.player_id = id;
            
            try {
                // Lee el comando
                protocol.read_command_client(comand_match);
            } catch (...) {
                break; // Socket cerrado o error
            }

            if (commands_queue) {
                try {
                    commands_queue->try_push(comand_match);
                } catch (const std::exception& e) {
                    std::cerr << "[Receiver] Error pushing command: " << e.what() << std::endl;
                    break;
                }
            }

            if (comand_match.command == GameCommand::DISCONNECT) {
                std::cout << "[Receiver]Player " << username << " sent DISCONNECT command" << std::endl;
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Match error: " << e.what() << std::endl;
    }
    std::cout << "[Receiver] Player " << username << " exited match " << match_id << std::endl;
}

void Receiver::run() {
    handle_lobby(); 
    
    // Si la conexión sigue viva y salimos del lobby, entramos al juego
    if (is_running && match_id != -1) {
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

void Receiver::kill() { is_running = false; }
bool Receiver::status() { return is_running; }
Receiver::~Receiver() {}