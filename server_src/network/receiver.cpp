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

        bool in_lobby = true;

        while (is_running && in_lobby) {
            uint8_t msg_type = protocol.read_message_type();

            switch (msg_type) {
            case MSG_LIST_GAMES: {
                // ... (Igual que antes) ...
                std::vector<GameInfo> games = monitor.list_available_matches();
                protocol.send_buffer(LobbyProtocol::serialize_games_list(games));
                break;
            }
            case MSG_CREATE_GAME: {
                // ... (Igual que antes) ...
                std::string game_name = protocol.read_string();
                uint8_t max_players = protocol.get_uint8_t();
                uint8_t num_races = protocol.get_uint8_t();
                
                int new_match_id = monitor.create_match(max_players, username, id, sender_messages_queue);
                if (new_match_id < 0) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Error"));
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
                }
                monitor.add_races_to_match(match_id, races);
                protocol.send_buffer(LobbyProtocol::serialize_game_created(match_id));
                
                std::vector<std::string> yaml_paths = monitor.get_race_paths(match_id);
                if (!yaml_paths.empty()) protocol.send_race_paths(yaml_paths);
                break;
            }
            case MSG_JOIN_GAME: {
                // ... (Igual que antes) ...
                int game_id = static_cast<int>(protocol.read_uint16());
                
                auto existing_players = monitor.get_match_players_snapshot(game_id);
                monitor.register_player_socket(game_id, username, protocol.get_socket());
                bool success = monitor.join_match(game_id, username, id, sender_messages_queue);

                if (!success) {
                    monitor.unregister_player_socket(game_id, username);
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_GAME_FULL, "Error"));
                    break;
                }
                current_match_id = game_id;
                this->match_id = game_id;

                protocol.send_buffer(LobbyProtocol::serialize_game_joined(static_cast<uint16_t>(game_id)));
                
                // Enviar snapshot
                for (const auto& [player_id, player_info] : existing_players) {
                    protocol.send_buffer(LobbyProtocol::serialize_player_joined_notification(player_info.name));
                    if (!player_info.car_name.empty()) 
                        protocol.send_buffer(LobbyProtocol::serialize_car_selected_notification(player_info.name, player_info.car_name, player_info.car_type));
                    if (player_info.is_ready)
                        protocol.send_buffer(LobbyProtocol::serialize_player_ready_notification(player_info.name, true));
                }
                std::vector<uint8_t> end_marker = {MSG_ROOM_SNAPSHOT, 0, 0};
                protocol.send_buffer(end_marker);

                std::vector<std::string> yaml_paths = monitor.get_race_paths(game_id);
                if (!yaml_paths.empty()) protocol.send_race_paths(yaml_paths);

                auto joined_notif = LobbyProtocol::serialize_player_joined_notification(username);
                monitor.broadcast_to_match(game_id, joined_notif, username);
                break;
            }
            case MSG_SELECT_CAR: {
                std::string car_name = protocol.read_string();
                std::string car_type = protocol.read_string();
                if (current_match_id == -1) break;
                if (!monitor.set_player_car(username, car_name, car_type)) break;
                protocol.send_buffer(LobbyProtocol::serialize_car_selected_ack(car_name, car_type));
                auto notif = LobbyProtocol::serialize_car_selected_notification(username, car_name, car_type);
                monitor.broadcast_to_match(current_match_id, notif, username);
                break;
            }
            case MSG_LEAVE_GAME: {
                uint16_t game_id = protocol.read_uint16();
                if (current_match_id != game_id) break;
                auto left_notif = LobbyProtocol::serialize_player_left_notification(username);
                monitor.broadcast_to_match(game_id, left_notif, username);
                monitor.leave_match(username);
                current_match_id = -1;
                this->match_id = -1;
                std::vector<GameInfo> games = monitor.list_available_matches();
                protocol.send_buffer(LobbyProtocol::serialize_games_list(games));
                break;
            }
            case MSG_PLAYER_READY: {
                uint8_t is_ready = protocol.get_uint8_t();
                std::cout << "[Receiver] " << username << " set ready: " << (is_ready ? "YES" : "NO") << "\n";
                if (current_match_id == -1) break;
                monitor.set_player_ready(username, is_ready != 0);
                auto notif = LobbyProtocol::serialize_player_ready_notification(username, is_ready != 0);
                monitor.broadcast_to_match(current_match_id, notif, username);
                break;
            }
            case MSG_START_GAME: {
                int game_id = static_cast<int>(protocol.read_uint16());
                std::cout << "[Receiver] " << username << " requesting START GAME " << game_id << "\n";
                if (current_match_id != game_id) break;

                if (!monitor.is_match_ready(game_id)) {
                    protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYERS_NOT_READY, "Not all ready"));
                    break;
                }
                
                std::vector<uint8_t> start_msg = { static_cast<uint8_t>(LobbyMessageType::MSG_GAME_STARTED) };
                
                // Enviar al Host (nosotros)
                protocol.send_buffer(start_msg);
                
                // Enviar a los demás
                monitor.broadcast_to_match(game_id, start_msg, username);
                std::cout << "[Receiver] 📡 MSG_GAME_STARTED enviado a todos.\n";

                // Iniciar lógica del servidor
                monitor.start_match(game_id);
                std::cout << "[Receiver] ✅ Match iniciado en servidor. Saliendo de Lobby.\n";
                
                in_lobby = false; // Host sale del lobby
                break;
            }
            default: {
                // INVITADOS: Si llega comando de juego y la partida empezó, salir del lobby
                if (current_match_id != -1 && monitor.is_match_started(current_match_id)) {
                    std::cout << "[Receiver] 🚀 Detectado comando de juego (" << (int)msg_type 
                              << ") en Lobby. Transicionando a GameLoop.\n";
                    in_lobby = false; 
                } else {
                    std::cerr << "[Receiver] Unknown message type: " << static_cast<int>(msg_type) << std::endl;
                    in_lobby = false; // Error real
                }
                break;
            }
            }
        } 

        std::cout << "[Receiver] ✅ Exiting lobby loop for " << username << std::endl;

        commands_queue = monitor.get_command_queue(match_id);
        sender.start();
        std::cout << "[Receiver] Sender started for player " << username << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Error: " << e.what() << std::endl;
        if (!username.empty() && current_match_id != -1) {
            try { monitor.leave_match(username); } catch(...) {}
        }
        is_running = false;
    }
}

void Receiver::handle_match_messages() {
    std::cout << "[Receiver] Game loop started - listening for commands...\n";
    try {
        while (is_running) {
            ComandMatchDTO comand_match;
            comand_match.player_id = id;
            try {
                protocol.read_command_client(comand_match);
            } catch (...) { break; }

            if (commands_queue) {
                try { commands_queue->try_push(comand_match); } catch (...) { break; }
            }
            if (comand_match.command == GameCommand::DISCONNECT) break;
        }
    } catch (...) {}
    std::cout << "[Receiver] Player " << username << " exited match\n";
}

void Receiver::run() {
    handle_lobby(); 
    if (is_running && match_id != -1) handle_match_messages(); 
    if (match_id != -1) monitor.delete_player_from_match(id, match_id);
    sender_messages_queue.close();
    if (match_id != -1) sender.join();
}

void Receiver::kill() { is_running = false; }
bool Receiver::status() { return is_running; }
Receiver::~Receiver() {}