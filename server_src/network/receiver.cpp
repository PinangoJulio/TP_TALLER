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
            std::cout << "[Receiver] Ciudad: " << nombre_ciudad << ", carreras encontradas: " << carreras.size() << "\n";
        }
    }
    return ciudades_y_carreras;
}


void Receiver::handle_lobby() {
    try {
        uint8_t msg_type_user = protocol.read_message_type();
        if (msg_type_user != MSG_USERNAME) {
            std::cerr << "[Receiver] Invalid protocol start (expected MSG_USERNAME)\n";
            return;
        }

        std::string username = protocol.read_string();
        std::cout << "[Receiver] Player connected: " << username << std::endl;

        auto welcome_msg = "Welcome to Need for Speed 2D, " + username + "!";
        protocol.send_buffer(LobbyProtocol::serialize_welcome(welcome_msg));

        // --- Paso 2: bucle principal del lobby ---
        bool in_lobby = true;
        int current_game_id = -1;  // 🔥 AGREGADO: Rastrear en qué partida está el jugador
        
        while (is_running && in_lobby) {
            uint8_t msg_type = protocol.read_message_type();

            switch (msg_type) {
                // ------------------------------------------------------------
                case MSG_LIST_GAMES: {
                    std::cout << "[Receiver] " << username << " requested games list\n";
                    auto games = monitor.list_available_matches();
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
                
                    if (current_game_id != -1) {
                        std::cout << "[Receiver] ERROR: " << username << " is already in game " << current_game_id << "\n";
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "You are already in a game"));
                        break;
                    }
                
                    int game_id = lobby_manager.create_game(game_name, username, max_players);
                    if (game_id == 0) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "Error creating match"));
                        break;
                    }
                
                    current_game_id = game_id;
                    
                    // 🔥 REGISTRAR SOCKET DEL HOST
                    lobby_manager.register_player_socket(game_id, username, protocol.get_socket());
                
                    // Recibir selección de carreras
                    std::vector<RaceConfig> races;
                    for (int i = 0; i < num_races; ++i) {
                        std::string city = protocol.read_string();
                        std::string map = protocol.read_string();
                        races.push_back({city, map});
                        std::cout << "[Receiver]   Race " << (i+1) << ": " << city << " - " << map << "\n";
                    }
                
                    // Aquí asumo que monitor tiene un método para agregar carreras
                    // Si no, necesitas adaptarlo según tu implementación
                    // monitor.add_races_to_match(game_id, races);
                    
                    protocol.send_buffer(LobbyProtocol::serialize_game_created(game_id));
                    std::cout << "[Receiver] Game created with ID: " << game_id << "\n";
                    break;
                }
                // ------------------------------------------------------------
                case MSG_JOIN_GAME: {
                    int game_id = static_cast<int>(protocol.read_uint16());
                    std::cout << "[Receiver] " << username << " joining game " << game_id << "\n";
                    
                    // Validar que no esté en otra partida
                    if (current_game_id != -1) {
                        std::cout << "[Receiver] ERROR: " << username << " is already in game " << current_game_id << "\n";
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "You are already in a game"));
                        break;
                    }
                
                    bool success = lobby_manager.join_game(game_id, username);
                
                    if (success) {
                        current_game_id = game_id;
                        
                        // 🔥 REGISTRAR SOCKET PARA BROADCAST
                        lobby_manager.register_player_socket(game_id, username, protocol.get_socket());
                        
                        protocol.send_buffer(LobbyProtocol::serialize_game_joined(static_cast<uint16_t>(game_id)));
                        std::cout << "[Receiver] " << username << " joined match " << game_id << std::endl;
                    } else {
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_GAME_FULL, "Game is full or started"));
                    }
                    break;
                }
                // ------------------------------------------------------------
                case MSG_SELECT_CAR: {
                    std::string car_name = protocol.read_string();
                    std::string car_type = protocol.read_string();
                    
                    std::cout << "[Receiver] " << username << " selected car: " 
                              << car_name << " (" << car_type << ")\n";
                    
                    monitor.set_player_car(id, car_name, car_type);
                    
                    // ENVIAR CONFIRMACIÓN AL CLIENTE
                    protocol.send_buffer(LobbyProtocol::serialize_car_selected_ack(car_name, car_type));
                    break;
                }
                // ------------------------------------------------------------
                // CORREGIDO: Handler completo para MSG_LEAVE_GAME
                case MSG_LEAVE_GAME: {
                    int game_id = static_cast<int>(protocol.read_uint16());
                    
                    std::cout << "[Receiver] " << username << " leaving game " << game_id << "\n";
                    
                    if (current_game_id != game_id) {
                        std::cout << "[Receiver] ERROR: " << username << " is not in game " << game_id 
                                  << " (current: " << current_game_id << ")\n";
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME, "You are not in that game"));
                        break;
                    }
                    
                    // Desregistrar socket
                    lobby_manager.unregister_player_socket(game_id, username);
                    
                    // Eliminar del manager
                    lobby_manager.leave_game(username);
                    
                    current_game_id = -1;
                    
                    std::vector<GameInfo> empty_list;
                    protocol.send_buffer(LobbyProtocol::serialize_games_list(empty_list));
                    
                    std::cout << "[Receiver] " << username << " successfully left game " << game_id << "\n";
                    break;
                }
                // ------------------------------------------------------------
                case MSG_PLAYER_READY: {
                    uint8_t is_ready = protocol.get_uint8_t();
                    
                    std::cout << "[Receiver] " << username << " set ready: " 
                              << (is_ready ? "YES" : "NO") << "\n";
                    
                    // 🔥 VALIDAR: ¿Está en una partida?
                    if (current_game_id == -1) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_PLAYER_NOT_IN_GAME, "You are not in any game"));
                        break;
                    }
                    
                    // Obtener la sala y cambiar estado
                    auto games = lobby_manager.get_all_games();
                    auto it = games.find(current_game_id);
                    if (it == games.end()) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_GAME_NOT_FOUND, "Game not found"));
                        break;
                    }
                    
                    GameRoom* room = it->second.get();
                    
                    // Cambiar estado ready (esto hará broadcast automático)
                    if (!room->set_player_ready(username, is_ready != 0)) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_INVALID_CAR_INDEX, "You must select a car before being ready"));
                    }
                    
                    break;
                }
                
                case MSG_START_GAME: {
                    int game_id = static_cast<int>(protocol.read_uint16());
                
                    std::cout << "[Receiver] " << username << " starting game " << game_id << "\n";
                    
                    // 🔥 VALIDAR: Es el host?
                    auto games = lobby_manager.get_all_games();
                    auto it = games.find(game_id);
                    if (it == games.end()) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_GAME_NOT_FOUND, "Game not found"));
                        break;
                    }
                    
                    GameRoom* room = it->second.get();
                    
                    if (!room->is_host(username)) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_NOT_HOST, "Only the host can start the game"));
                        break;
                    }
                    
                    if (!room->all_players_ready()) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_PLAYERS_NOT_READY, "Not all players are ready"));
                        break;
                    }
                    
                    // Iniciar la partida
                    room->start();
                    
                    // Broadcast MSG_GAME_STARTED a todos
                    auto buffer = LobbyProtocol::serialize_game_started(static_cast<uint16_t>(game_id));
                    if (room->get_broadcast_callback()) {
                        room->get_broadcast_callback()(buffer);
                    }
                    
                    in_lobby = false;
                    this->match_id = game_id;
                    current_game_id = -1;
                    
                    // Iniciar el juego
                    monitor.start_match(game_id);
                    break;
                }
                // ------------------------------------------------------------
                default:
                    std::cerr << "[Receiver] Unknown message type: " << static_cast<int>(msg_type) << std::endl;
                    in_lobby = false;
                    break;
            }
        }

        if (!in_lobby) {
            std::cout << "[Receiver] Player " << username << " entering match " << match_id << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Lobby error: " << e.what() << std::endl;
        is_running = false;
    }
}


void Receiver::handle_match_messages() {
    //implementar comunicacion de juego
    is_running = false;
}


void Receiver::run() {
    handle_lobby();
    // handle_match_messages
}

void Receiver::kill() { is_running = false; }

bool Receiver::status() { return is_running; }

Receiver::~Receiver() {}