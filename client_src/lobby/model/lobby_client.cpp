#include "lobby_client.h"
#include <iostream>
#include <netinet/in.h>

LobbyClient::LobbyClient(const std::string& host, const std::string& port)
    : socket(host.c_str(), port.c_str()), connected(true) {
    std::cout << "[LobbyClient] Connected to server " << host << ":" << port << std::endl;
}

uint8_t LobbyClient::read_message_type() {
    uint8_t type;
    int bytes = socket.recvall(&type, sizeof(type));
    if (bytes == 0) {
        connected = false;
        throw std::runtime_error("Connection closed by server");
    }
    return type;
}

std::string LobbyClient::read_string() {
    uint16_t len_net;
    socket.recvall(&len_net, sizeof(len_net));
    uint16_t len = ntohs(len_net);
    
    std::vector<char> buffer(len);
    socket.recvall(buffer.data(), len);
    
    return std::string(buffer.begin(), buffer.end());
}

uint16_t LobbyClient::read_uint16() {
    uint16_t value_net;
    socket.recvall(&value_net, sizeof(value_net));
    return ntohs(value_net);
}

void LobbyClient::send_username(const std::string& user) {
    this->username = user;
    auto buffer = LobbyProtocol::serialize_username(username);
    
    std::cout << "[LobbyClient] DEBUG: Username: '" << username 
              << "' (length: " << username.length() << ")" << std::endl;
    std::cout << "[LobbyClient] DEBUG: Buffer size: " << buffer.size() << " bytes" << std::endl;
    std::cout << "[LobbyClient] DEBUG: Buffer content: ";
    for (size_t i = 0; i < buffer.size(); ++i) {
        printf("%02X ", buffer[i]);
    }
    std::cout << std::endl;
    
    if (buffer.size() >= 3) {
        uint16_t len_sent = (buffer[1] << 8) | buffer[2];
        std::cout << "[LobbyClient] DEBUG: Length encoded in buffer: " << len_sent << std::endl;
    }
    
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Sent username: " << username << std::endl;
}

std::string LobbyClient::receive_welcome() {
    std::cout << "[LobbyClient] DEBUG: Waiting for welcome message..." << std::endl;
    uint8_t type = read_message_type();
    std::cout << "[LobbyClient] DEBUG: Received type: " << static_cast<int>(type) << std::endl;
    
    if (type != MSG_WELCOME) {
        throw std::runtime_error("Expected WELCOME message");
    }
    
    std::cout << "[LobbyClient] DEBUG: Reading welcome string..." << std::endl;
    std::string message = read_string();
    std::cout << "[LobbyClient] Received welcome: " << message << std::endl;
    return message;
}

void LobbyClient::request_games_list() {
    auto buffer = LobbyProtocol::serialize_list_games();
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Requested games list" << std::endl;
}

std::vector<GameInfo> LobbyClient::receive_games_list() {
    uint8_t type = read_message_type();
    if (type != MSG_GAMES_LIST) {
        throw std::runtime_error("Expected GAMES_LIST message");
    }
    
    uint16_t count = read_uint16();
    std::vector<GameInfo> games;
    
    for (uint16_t i = 0; i < count; ++i) {
        GameInfo info;
        info.game_id = read_uint16();
        socket.recvall(info.game_name, sizeof(info.game_name));
        socket.recvall(&info.current_players, sizeof(info.current_players));
        socket.recvall(&info.max_players, sizeof(info.max_players));
        
        uint8_t started;
        socket.recvall(&started, sizeof(started));
        info.is_started = (started != 0);
        
        games.push_back(info);
    }
    
    std::cout << "[LobbyClient] Received " << games.size() << " games" << std::endl;
    return games;
}

void LobbyClient::send_string(const std::string& str) {
    uint16_t len = htons(static_cast<uint16_t>(str.size()));
    socket.sendall(&len, sizeof(len));        // Enviamos longitud en orden de red
    socket.sendall(str.data(), str.size());   // Enviamos los bytes del string
}


void LobbyClient::create_game(const std::string& game_name, uint8_t max_players, const std::vector<std::pair<std::string,std::string>>& races) {
    auto buffer = LobbyProtocol::serialize_create_game(game_name, max_players, races.size());
    socket.sendall(buffer.data(), buffer.size());
    for (const auto& race : races) {
        send_string(race.first);   // city
        send_string(race.second);
    }
    std::cout << "[LobbyClient] Requested to create game: " << game_name
              << " (max players: " << static_cast<int>(max_players)
              << ", races: " << static_cast<int>(races.size()) << ")\n";
}

uint16_t LobbyClient::receive_game_created() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_CREATED) {
        throw std::runtime_error("Expected GAME_CREATED message");
    }
    
    uint16_t game_id = read_uint16();
    std::cout << "[LobbyClient] Game created with ID: " << game_id << std::endl;
    return game_id;
}

void LobbyClient::join_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_join_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Requested to join game: " << game_id << std::endl;
}

uint16_t LobbyClient::receive_game_joined() {
    uint8_t type = read_message_type();
    if (type != MSG_GAME_JOINED) {
        throw std::runtime_error("Expected GAME_JOINED message");
    }
    
    uint16_t game_id = read_uint16();
    std::cout << "[LobbyClient] Joined game: " << game_id << std::endl;
    
    // 🔥 ELIMINADO: receive_room_snapshot()
    // El snapshot llegará vía notificaciones automáticamente
    
    return game_id;
}

void LobbyClient::receive_room_snapshot() {
    uint8_t type = read_message_type();
    if (type != MSG_ROOM_SNAPSHOT) {
        throw std::runtime_error("Expected ROOM_SNAPSHOT message");
    }
    
    uint16_t player_count = read_uint16();
    
    for (int i = 0; i < player_count; i++) {
        std::string player_name = read_string();
        std::string car_name = read_string();
        std::string car_type = read_string();
        uint8_t is_ready = read_uint8();
        
        // Emitir señales para actualizar la UI
        emit playerJoinedNotification(QString::fromStdString(player_name));
        
        if (!car_name.empty()) {
            emit carSelectedNotification(
                QString::fromStdString(player_name),
                QString::fromStdString(car_name),
                QString::fromStdString(car_type)
            );
        }
        
        if (is_ready) {
            emit playerReadyNotification(QString::fromStdString(player_name), true);
        }
    }
    
    std::cout << "[LobbyClient] Room snapshot received (" << player_count << " players)" << std::endl;
}

uint8_t LobbyClient::peek_message_type() {
    return read_message_type();
}

void LobbyClient::read_error_details(std::string& error_message) {
    uint8_t error_code;
    socket.recvall(&error_code, sizeof(error_code));
    
    error_message = read_string();
    
    std::cout << "[LobbyClient] Error received (code " << static_cast<int>(error_code) 
              << "): " << error_message << std::endl;
}

uint8_t LobbyClient::read_uint8() {
    uint8_t value;
    socket.recvall(&value, sizeof(value));
    return value;
}

std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> LobbyClient::receive_city_maps() {
    uint8_t type = read_message_type();
    if (type != MSG_CITY_MAPS) {
        throw std::runtime_error("Expected CITY_MAPS message");
    }

    uint16_t num_cities = read_uint16();
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;

    for (int i = 0; i < num_cities; ++i) {
        std::string city_name = read_string();
        uint16_t num_maps = read_uint16();

        std::vector<std::pair<std::string, std::string>> maps;
        for (int j = 0; j < num_maps; ++j) {
            std::string yaml = read_string();
            std::string png = read_string();
            maps.emplace_back(yaml, png);
        }

        result.emplace_back(city_name, maps);
    }

    return result;
}

void LobbyClient::send_selected_races(const std::vector<std::pair<std::string, std::string>>& races) {
    for (const auto& [city, map] : races) {
        auto buffer_city = LobbyProtocol::serialize_string(city);
        auto buffer_map = LobbyProtocol::serialize_string(map);
        socket.sendall(buffer_city.data(), buffer_city.size());
        socket.sendall(buffer_map.data(), buffer_map.size());
        std::cout << "[LobbyClient] Selected race: " << city << " - " << map << std::endl;
    }
}

void LobbyClient::select_car(const std::string& car_name, const std::string& car_type) {
    auto buffer = LobbyProtocol::serialize_select_car(car_name, car_type);  
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Selected car: " << car_name << " (" << car_type << ")\n";
}

std::string LobbyClient::receive_car_confirmation() {
    uint8_t type = read_message_type();
    if (type != MSG_CAR_SELECTED_ACK) {  
        throw std::runtime_error("Expected CAR_SELECTED_ACK");
    }
    std::string car_name = read_string();
    std::string car_type = read_string();
    
    std::cout << "[LobbyClient] Server confirmed car: " << car_name << " (" << car_type << ")" << std::endl;
    return car_name;
}

void LobbyClient::start_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_game_started(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Start game request sent for ID: " << game_id << std::endl;
}

void LobbyClient::leave_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_leave_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Leave game request sent for ID: " << game_id << std::endl;
}


void LobbyClient::start_listening() {
    if (listening.load()) {
        std::cout << "[LobbyClient] Already listening" << std::endl;
        return;
    }
    
    listening.store(true);
    notification_thread = std::thread(&LobbyClient::notification_listener, this);
    std::cout << "[LobbyClient] Notification listener started" << std::endl;
}

void LobbyClient::stop_listening() {
    if (!listening.load()) {
        std::cout << "[LobbyClient] Listener already stopped" << std::endl;
        return;
    }
    
    std::cout << "[LobbyClient] Stopping notification listener..." << std::endl;
    
    // 🔥 PASO 1: Marcar como detenido
    listening.store(false);
    
    // 🔥 PASO 2: Esperar a que el thread termine (con timeout)
    if (notification_thread.joinable()) {
        std::cout << "[LobbyClient] Waiting for listener thread to finish..." << std::endl;
        
        // Dar 3 segundos máximo
        auto start = std::chrono::steady_clock::now();
        while (notification_thread.joinable()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start
            ).count();
            
            if (elapsed > 3000) {
                std::cerr << "[LobbyClient] ⚠️  Timeout waiting for listener!" << std::endl;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (notification_thread.joinable()) {
            notification_thread.join();
            std::cout << "[LobbyClient] Notification listener joined" << std::endl;
        }
    }
    
    std::cout << "[LobbyClient] Notification listener stopped" << std::endl;
}

void LobbyClient::notification_listener() {
    std::cout << "[LobbyClient] Notification listener running..." << std::endl;
    
    try {
        while (listening.load() && connected) {
            uint8_t msg_type;
            
            try {
                msg_type = read_message_type();
            } catch (const std::exception& e) {
                if (!listening.load()) {
                    std::cout << "[LobbyClient] Listener stopped gracefully" << std::endl;
                    break;
                }
                throw;
            }
            
            // 🔥 Si estamos cerrando, ignorar todos los mensajes
            if (!listening.load()) {
                std::cout << "[LobbyClient] Listener stopped, ignoring message type " 
                          << static_cast<int>(msg_type) << std::endl;
                break;
            }
            
            std::cout << "[LobbyClient] Received notification type: " 
                      << static_cast<int>(msg_type) << std::endl;
            
            switch (msg_type) {
                case MSG_PLAYER_JOINED_NOTIFICATION: {
                    std::string username = read_string();
                    std::cout << "[LobbyClient] Player joined: " << username << std::endl;
                    emit playerJoinedNotification(QString::fromStdString(username));
                    break;
                }
                
                case MSG_PLAYER_LEFT_NOTIFICATION: {
                    std::string username = read_string();
                    std::cout << "[LobbyClient] Player left: " << username << std::endl;
                    emit playerLeftNotification(QString::fromStdString(username));
                    break;
                }
                
                case MSG_PLAYER_READY_NOTIFICATION: {
                    std::string username = read_string();
                    uint8_t is_ready = read_uint8();
                    std::cout << "[LobbyClient] Player " << username 
                              << " is now " << (is_ready ? "READY" : "NOT READY") << std::endl;
                    emit playerReadyNotification(
                        QString::fromStdString(username), 
                        is_ready != 0
                    );
                    break;
                }
                
                case MSG_CAR_SELECTED_NOTIFICATION: {
                    std::string username = read_string();
                    std::string car_name = read_string();
                    std::string car_type = read_string();
                    std::cout << "[LobbyClient] Player " << username 
                              << " selected " << car_name << std::endl;
                    emit carSelectedNotification(
                        QString::fromStdString(username),
                        QString::fromStdString(car_name),
                        QString::fromStdString(car_type)
                    );
                    break;
                }
                
                case MSG_GAMES_LIST: {
                    std::cout << "[LobbyClient] Received MSG_GAMES_LIST in listener (consuming fully)" << std::endl;
                    
                    uint16_t count = read_uint16();
                    std::cout << "[LobbyClient] Games list has " << count << " games" << std::endl;
                    
                    // 🔥 Leer TODOS los juegos
                    std::vector<GameInfo> games;
                    for (uint16_t i = 0; i < count; i++) {
                        GameInfo info;
                        info.game_id = read_uint16();  
                        socket.recvall(info.game_name, sizeof(info.game_name));  
                        socket.recvall(&info.current_players, sizeof(info.current_players));  
                        socket.recvall(&info.max_players, sizeof(info.max_players));  
                        uint8_t started;
                        socket.recvall(&started, sizeof(started));
                        info.is_started = (started != 0);
                        
                        games.push_back(info);
                        std::cout << "[LobbyClient]   Game " << info.game_id << ": " << info.game_name << std::endl;
                    }
                    
                    // 🔥 EMITIR SEÑAL para actualizar la UI
                    emit gamesListReceived(games);
                    
                    std::cout << "[LobbyClient] MSG_GAMES_LIST fully consumed, exiting listener" << std::endl;
                    
                    // Salir del listener
                    listening.store(false);
                    return;
                }
                
                case MSG_ERROR: {
                    uint8_t error_code = read_uint8();
                    std::string error_msg = read_string();
                    std::cerr << "[LobbyClient] Error " << static_cast<int>(error_code) 
                              << ": " << error_msg << std::endl;
                    emit errorOccurred(QString::fromStdString(error_msg));
                    break;
                }
                
                default:
                    std::cerr << "[LobbyClient] Unknown notification type: " 
                              << static_cast<int>(msg_type) << std::endl;
                    break;
            }
        }
    } catch (const std::exception& e) {
        if (listening.load()) {
            std::cerr << "[LobbyClient] Notification listener error: " << e.what() << std::endl;
        }
        connected = false;
    }
    
    std::cout << "[LobbyClient] Notification listener exited" << std::endl;
}

void LobbyClient::set_ready(bool is_ready) {
    auto buffer = LobbyProtocol::serialize_player_ready(is_ready);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Set ready: " << (is_ready ? "YES" : "NO") << std::endl;
}

void LobbyClient::read_room_snapshot(std::vector<QString>& players, 
                                      std::map<QString, QString>& cars) {
    std::cout << "[LobbyClient] Reading room snapshot manually..." << std::endl;
    
    // 🔥 FIX: Leer hasta encontrar el marcador MSG_ROOM_SNAPSHOT con count=0
    while (true) {
        uint8_t msg_type;
        
        try {
            msg_type = read_message_type();
        } catch (const std::exception& e) {
            std::cerr << "[LobbyClient] Error reading snapshot: " << e.what() << std::endl;
            break;
        }
        
        std::cout << "[LobbyClient] Snapshot msg type: " << static_cast<int>(msg_type) << std::endl;
        
        // 🔥 DETECTAR FIN DE SNAPSHOT
        if (msg_type == MSG_ROOM_SNAPSHOT) {
            uint8_t count1, count2;
            socket.recvall(&count1, sizeof(count1));
            socket.recvall(&count2, sizeof(count2));
            
            if (count1 == 0 && count2 == 0) {
                std::cout << "[LobbyClient] ✅ END OF SNAPSHOT detected" << std::endl;
                break;  // Fin del snapshot
            }
        }
        
        // Procesar mensajes de snapshot
        if (msg_type == MSG_PLAYER_JOINED_NOTIFICATION) {
            std::string username = read_string();
            players.push_back(QString::fromStdString(username));
            std::cout << "[LobbyClient]   Snapshot player: " << username << std::endl;
            
        } else if (msg_type == MSG_CAR_SELECTED_NOTIFICATION) {
            std::string username = read_string();
            std::string car_name = read_string();
            std::string car_type = read_string();
            cars[QString::fromStdString(username)] = QString::fromStdString(car_name);
            std::cout << "[LobbyClient]   Snapshot car: " << username << " -> " << car_name << std::endl;
            
        } else if (msg_type == MSG_PLAYER_READY_NOTIFICATION) {
            std::string username = read_string();
            uint8_t is_ready = read_uint8();
            std::cout << "[LobbyClient]   Snapshot ready: " << username << " -> " << (is_ready ? "YES" : "NO") << std::endl;
        }
    }
    
    std::cout << "[LobbyClient] Snapshot read complete: " << players.size() << " players" << std::endl;
}

// Destructor actualizado
LobbyClient::~LobbyClient() {
    stop_listening();
}


// ========================================================================================================================================================================
// Estas son las funciones que necesito nuevas para seguir

// void LobbyClient::leave_game(uint16_t game_id) {
    
//     // To do: Falta esto en el servidor
// }

// void LobbyClient::select_car(uint8_t car_index) {
    
//     // To do: Falta esto en el servidor
// }

// void LobbyClient::set_ready(bool is_ready) {
    
//     // To do: Falta esto en el servidor
// }

// void LobbyClient::start_game(uint16_t game_id) {
    
    
//     // To do: Falta esto en el servidor
// }
// // ========================================================================================================================================================================