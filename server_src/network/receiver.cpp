#include "receiver.h"
#include <filesystem>
#include <iostream>
#include <sys/socket.h>
#include <cstring>

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
    int current_game_id = -1;
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

        // --- Paso 2: bucle principal del lobby ---
        bool in_lobby = true;
        int current_game_id = -1;  // 🔥 AGREGADO: Rastrear en qué partida está el jugador
        
        while (is_running && in_lobby) {
            uint8_t msg_type = protocol.read_message_type();

            switch (msg_type) {
                // ------------------------------------------------------------
                case MSG_LIST_GAMES: {
                    std::cout << "[Receiver] " << username << " requested games list\n";
                    
                    // 🔥 CAMBIO: Usar lobby_manager en lugar de monitor
                    std::vector<GameInfo> games;
                    
                    for (const auto& [game_id, room] : lobby_manager.get_all_games()) {
                        GameInfo info{};
                        info.game_id = game_id;
                        
                        // Copiar nombre del juego
                        std::string name = room->get_game_name();
                        size_t copy_len = std::min(name.length(), sizeof(info.game_name) - 1);
                        std::memcpy(info.game_name, name.c_str(), copy_len);
                        info.game_name[copy_len] = '\0';
                        
                        info.current_players = room->get_player_count();
                        info.max_players = room->get_max_players();
                        info.is_started = room->is_started();
                        
                        games.push_back(info);
                    }
                    
                    std::cout << "[Receiver] Sending " << games.size() << " games to " << username << "\n";
                    
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
                    
                    if (current_game_id != -1) {
                        std::cout << "[Receiver] ERROR: " << username << " is already in game " << current_game_id << "\n";
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_ALREADY_IN_GAME, "You are already in a game"));
                        break;
                    }
                
                    // 🔥 1. OBTENER LA SALA
                    auto& games = lobby_manager.get_all_games();
                    auto room_it = games.find(game_id);
                    if (room_it == games.end()) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_GAME_NOT_FOUND, "Game not found"));
                        break;
                    }
                    
                    GameRoom* room = room_it->second.get();
                
                    // 🔥 2. REGISTRAR SOCKET **ANTES** DE JOIN
                    lobby_manager.register_player_socket(game_id, username, protocol.get_socket());
                    
                    // 🔥 3. TEMPORALMENTE DESACTIVAR EL CALLBACK DE BROADCAST
                    auto original_callback = room->get_broadcast_callback();
                    room->set_broadcast_callback(nullptr);
                    
                    // 🔥 4. HACER JOIN (sin que dispare broadcast todavía)
                    bool success = lobby_manager.join_game(game_id, username);
                
                    if (success) {
                        current_game_id = game_id;
                        
                        // 🔥 5. ENVIAR CONFIRMACIÓN AL NUEVO JUGADOR **PRIMERO**
                        protocol.send_buffer(LobbyProtocol::serialize_game_joined(static_cast<uint16_t>(game_id)));
                        std::cout << "[Receiver] " << username << " joined match " << game_id << std::endl;
                        
                        // 🔥 6. RESTAURAR CALLBACK Y HACER BROADCAST MANUAL
                        room->set_broadcast_callback(original_callback);
                        
                        // Broadcast a los DEMÁS jugadores (no al que acaba de unirse)
                        auto joined_notif = LobbyProtocol::serialize_player_joined_notification(username);
                        if (original_callback) {
                            original_callback(joined_notif);
                        }
                        
                        // 🔥 7. AHORA SÍ, ENVIAR SNAPSHOT AL NUEVO JUGADOR
                        const auto& existing_players = room->get_players();
                        
                        std::cout << "[Receiver] Sending room snapshot to " << username 
                                  << " (" << existing_players.size() << " players)" << std::endl;
                        
                        for (const auto& [player_name, player_info] : existing_players) {
                            if (player_name == username) continue;
                            
                            auto joined_notif = LobbyProtocol::serialize_player_joined_notification(player_name);
                            protocol.send_buffer(joined_notif);
                            
                            std::cout << "[Receiver]   → Player exists: " << player_name << std::endl;
                            
                            if (!player_info.car_name.empty()) {
                                auto car_notif = LobbyProtocol::serialize_car_selected_notification(
                                    player_name, 
                                    player_info.car_name, 
                                    player_info.car_type
                                );
                                protocol.send_buffer(car_notif);
                                
                                std::cout << "[Receiver]   → Car: " << player_info.car_name << std::endl;
                            }
                            
                            if (player_info.is_ready) {
                                auto ready_notif = LobbyProtocol::serialize_player_ready_notification(
                                    player_name, 
                                    true
                                );
                                protocol.send_buffer(ready_notif);
                                
                                std::cout << "[Receiver]   → Ready: YES" << std::endl;
                            }
                        }
                        
                    } else {
                        lobby_manager.unregister_player_socket(game_id, username);
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
                    
                    if (current_game_id == -1) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_PLAYER_NOT_IN_GAME, "You are not in any game"));
                        break;
                    }
                    
                    auto& games = lobby_manager.get_all_games();
                    auto it = games.find(current_game_id);
                    if (it == games.end()) {
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_GAME_NOT_FOUND, "Game not found"));
                        break;
                    }
                    
                    GameRoom* room = it->second.get();
                    
                    // 🔥 PASO 1: Desactivar callback de broadcast temporalmente
                    auto original_callback = room->get_broadcast_callback();
                    room->set_broadcast_callback(nullptr);
                    
                    // 🔥 PASO 2: Guardar el auto (SIN broadcast)
                    if (!room->set_player_car(username, car_name, car_type)) {
                        room->set_broadcast_callback(original_callback);
                        protocol.send_buffer(LobbyProtocol::serialize_error(
                            ERR_INVALID_CAR_INDEX, "Failed to select car"));
                        break;
                    }
                    
                    // 🔥 PASO 3: Restaurar callback
                    room->set_broadcast_callback(original_callback);
                    
                    // 🔥 PASO 4: Enviar ACK al cliente que seleccionó (PRIMERO)
                    protocol.send_buffer(LobbyProtocol::serialize_car_selected_ack(car_name, car_type));
                    std::cout << "[Receiver] ✅ ACK sent to " << username << std::endl;
                    
                    // 🔥 PASO 5: AHORA SÍ hacer broadcast a TODOS (incluyendo al que seleccionó)
                    if (original_callback) {
                        auto notif = LobbyProtocol::serialize_car_selected_notification(username, car_name, car_type);
                        original_callback(notif);
                        std::cout << "[Receiver] ✅ Broadcast triggered" << std::endl;
                    }
                    
                    break;
                }
                // ------------------------------------------------------------
                case MSG_LEAVE_GAME: {
                    uint16_t game_id = protocol.read_uint16();
                    
                    std::cout << "[Receiver] " << username << " leaving game " << game_id << "\n";
                    
                    // Validar que esté en esa partida
                    if (current_game_id != game_id) {
                        std::cout << "[Receiver] ERROR: " << username << " is not in game " << game_id 
                                  << " (current: " << current_game_id << ")\n";
                        protocol.send_buffer(LobbyProtocol::serialize_error(ERR_PLAYER_NOT_IN_GAME, "You are not in that game"));
                        break;
                    }
                    
                    // 🔥 LIMPIAR ESTADO LOCAL **PRIMERO**
                    current_game_id = -1;
                    
                    // Desregistrar socket y eliminar del manager
                    lobby_manager.leave_game(username);
                    
                    // Enviar lista de partidas actualizada
                    std::vector<GameInfo> games;
                    for (const auto& [gid, room] : lobby_manager.get_all_games()) {
                        GameInfo info{};
                        info.game_id = gid;
                        
                        std::string name = room->get_game_name();
                        size_t copy_len = std::min(name.length(), sizeof(info.game_name) - 1);
                        std::memcpy(info.game_name, name.c_str(), copy_len);
                        info.game_name[copy_len] = '\0';
                        
                        info.current_players = room->get_player_count();
                        info.max_players = room->get_max_players();
                        info.is_started = room->is_started();
                        
                        games.push_back(info);
                    }
                    
                    protocol.send_buffer(LobbyProtocol::serialize_games_list(games));
                    
                    std::cout << "[Receiver] " << username << " successfully left game " << game_id 
                              << ", sent " << games.size() << " available games\n";
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
                    auto& games = lobby_manager.get_all_games();
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
                    
                    // Validaciones...
                    auto& games = lobby_manager.get_all_games();
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
                    
                    std::cout << "[Receiver] Game " << game_id << " started successfully!" << std::endl;
                    
                    // 🔥 NO CAMBIAR in_lobby TODAVÍA (el juego aún no está implementado)
                    // in_lobby = false;  // ← COMENTAR ESTO
                    // this->match_id = game_id;
                    // current_game_id = -1;
                    // monitor.start_match(game_id);
                    
                    // 🔥 POR AHORA, MANTENER EN LOBBY PARA QUE PUEDAN SALIR
                    std::cout << "[Receiver] WARNING: Game logic not implemented, staying in lobby" << std::endl;
                    
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
        std::string error_msg = e.what();
        
        // 🔥 DISTINGUIR ENTRE DESCONEXIÓN NORMAL Y ERROR
        if (error_msg.find("Connection closed") != std::string::npos) {
            std::cout << "[Receiver] Player " << username << " disconnected" << std::endl;
        } else {
            std::cerr << "[Receiver] Lobby error: " << error_msg << std::endl;
        }
        
        // 🔥 LIMPIEZA: Eliminar jugador de su partida si estaba en una
        if (!username.empty() && current_game_id != -1) {
            std::cout << "[Receiver] Cleaning up " << username 
                      << " from game " << current_game_id << " (disconnect)" << std::endl;
            
            try {
                lobby_manager.leave_game(username);
                std::cout << "[Receiver] ✅ " << username << " cleaned up successfully" << std::endl;
            } catch (const std::exception& cleanup_error) {
                std::cerr << "[Receiver] ❌ Failed to cleanup: " << cleanup_error.what() << std::endl;
            }
        }
        
        is_running = false;
    }
}


void Receiver::handle_match_messages() {
    std::cout << "[Receiver] Player " << username << " entered match " << match_id << std::endl;
    std::cout << "[Receiver] Match communication not implemented yet, keeping connection alive..." << std::endl;
    
    // 🔥 NO CERRAR LA CONEXIÓN, mantener el thread esperando
    // (esto se implementará cuando agregues la lógica del juego)
    
    try {
        while (is_running) {
            // 🔥 Por ahora, simplemente esperar comandos del juego
            // (en el futuro aquí irán los comandos de movimiento, etc.)
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        std::cerr << "[Receiver] Match error: " << e.what() << std::endl;
    }
    
    std::cout << "[Receiver] Player " << username << " left match " << match_id << std::endl;
}


void Receiver::run() {
    handle_lobby();
    // handle_match_messages
}

void Receiver::kill() { is_running = false; }

bool Receiver::status() { return is_running; }

Receiver::~Receiver() {}