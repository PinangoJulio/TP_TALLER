#include "lobby_client.h"

#include <netinet/in.h>

#include <iostream>
#include <map>
#include <utility>

LobbyClient::LobbyClient(ClientProtocol& protocol)
    : protocol(protocol), connected(true) {
    std::cout << "[LobbyClient] Connected to server " << std::endl;
}

void LobbyClient::send_username(const std::string& user) {
    this->username = user;
    protocol.send_username(username);
}

std::string LobbyClient::receive_welcome() {
    std::cout << "[LobbyClient] DEBUG: Waiting for welcome message..." << std::endl;
    uint8_t type = protocol.read_message_type();
    std::cout << "[LobbyClient] DEBUG: Received type: " << static_cast<int>(type) << std::endl;

    if (type != MSG_WELCOME) {
        throw std::runtime_error("Expected WELCOME message");
    }

    std::cout << "[LobbyClient] DEBUG: Reading welcome string..." << std::endl;
    std::string message = protocol.read_string();
    std::cout << "[LobbyClient] Received welcome: " << message << std::endl;
    return message;
}

// esta
void LobbyClient::request_games_list() {
    protocol.request_games_list();
}

// esta
std::vector<GameInfo> LobbyClient::receive_games_list() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_GAMES_LIST) {
        throw std::runtime_error("Expected GAMES_LIST message");
    }

    uint16_t count = protocol.read_uint16();
    std::vector<GameInfo> games = protocol.read_games_list_from_socket(count);

    std::cout << "[LobbyClient] Received " << games.size() << " games" << std::endl;
    return games;
}

// esta
void LobbyClient::create_game(const std::string& game_name, uint8_t max_players,
                              const std::vector<std::pair<std::string, std::string>>& races) {
    protocol.create_game(game_name, max_players, races);
}

// esta
uint16_t LobbyClient::receive_game_created() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_GAME_CREATED) {
        throw std::runtime_error("Expected GAME_CREATED message");
    }

    uint16_t game_id = protocol.read_uint16();
    std::cout << "[LobbyClient] Game created with ID: " << game_id << std::endl;
    return game_id;
}

// esta
void LobbyClient::join_game(uint16_t game_id) {
    protocol.join_game(game_id);
}

// esta
uint16_t LobbyClient::receive_game_joined() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_GAME_JOINED) {
        throw std::runtime_error("Expected GAME_JOINED message");
    }

    uint16_t game_id = protocol.read_uint16();
    std::cout << "[LobbyClient] Joined game: " << game_id << std::endl;

    // El snapshot llegará vía notificaciones automáticamente

    return game_id;
}

void LobbyClient::receive_room_snapshot() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_ROOM_SNAPSHOT) {
        throw std::runtime_error("Expected ROOM_SNAPSHOT message");
    }

    uint16_t player_count = protocol.read_uint16();

    for (int i = 0; i < player_count; i++) {
        std::string player_name = protocol.read_string();
        std::string car_name = protocol.read_string();
        std::string car_type = protocol.read_string();
        uint8_t is_ready = protocol.read_uint8();

        // Emitir señales para actualizar la UI
        emit playerJoinedNotification(QString::fromStdString(player_name));

        if (!car_name.empty()) {
            emit carSelectedNotification(QString::fromStdString(player_name),
                                         QString::fromStdString(car_name),
                                         QString::fromStdString(car_type));
        }

        if (is_ready) {
            emit playerReadyNotification(QString::fromStdString(player_name), true);
        }
    }

    std::cout << "[LobbyClient] Room snapshot received (" << player_count << " players)"
              << std::endl;
}

uint8_t LobbyClient::peek_message_type() {
    return protocol.read_message_type();
}

// esta
void LobbyClient::read_error_details(std::string& error_message) {
    protocol.read_error_details(error_message);
}

// esta
std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>>
LobbyClient::receive_city_maps() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_CITY_MAPS) {
        throw std::runtime_error("Expected CITY_MAPS message");
    }

    uint16_t num_cities = protocol.read_uint16();
    std::vector<std::pair<std::string, std::vector<std::pair<std::string, std::string>>>> result;

    for (int i = 0; i < num_cities; ++i) {
        std::string city_name = protocol.read_string();
        uint16_t num_maps = protocol.read_uint16();

        std::vector<std::pair<std::string, std::string>> maps;
        for (int j = 0; j < num_maps; ++j) {
            std::string yaml = protocol.read_string();
            std::string png = protocol.read_string();
            maps.emplace_back(yaml, png);
        }

        result.emplace_back(city_name, maps);
    }

    return result;
}

// esta
void LobbyClient::send_selected_races(
    const std::vector<std::pair<std::string, std::string>>& races) {
    protocol.send_selected_races(races);
}

// esta
void LobbyClient::select_car(const std::string& car_name, const std::string& car_type) {
    protocol.select_car(car_name, car_type);
}

std::string LobbyClient::receive_car_confirmation() {
    uint8_t type = protocol.read_message_type();
    if (type != MSG_CAR_SELECTED_ACK) {
        throw std::runtime_error("Expected CAR_SELECTED_ACK");
    }
    std::string car_name = protocol.read_string();
    std::string car_type = protocol.read_string();

    std::cout << "[LobbyClient] Server confirmed car: " << car_name << " (" << car_type << ")"
              << std::endl;
    return car_name;
}

// esta
void LobbyClient::start_game(uint16_t game_id) {
    protocol.start_game(game_id);
}

// esta
void LobbyClient::leave_game(uint16_t game_id) {
    protocol.leave_game(game_id);
}

void LobbyClient::start_listening() {
    // Si el listener ya está corriendo, NO hacer nada
    if (listening.load()) {
        std::cout << "[LobbyClient] Listener is already running, skipping start" << std::endl;
        return;
    }

    // FIX: Si hay un thread anterior, asegurarse de que terminó
    if (notification_thread.joinable()) {
        std::cout << "[LobbyClient] ⚠️  Previous listener thread still exists, joining..."
                  << std::endl;
        notification_thread.join();
        std::cout << "[LobbyClient] ✅ Previous thread cleaned up" << std::endl;
    }

    listening.store(true);
    notification_thread = std::thread(&LobbyClient::notification_listener, this);
    std::cout << "[LobbyClient] Notification listener started" << std::endl;
}

void LobbyClient::stop_listening() {
    std::cout << "[LobbyClient] Stopping notification listener..." << std::endl;

    // Marcar como detenido
    listening.store(false);

    // Hacer shutdown del socket para desbloquear cualquier recv() pendiente
    try {
        protocol.shutdown_socket();
        std::cout << "[LobbyClient] Socket shutdown to unblock listener" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[LobbyClient] Error shutting down socket: " << e.what() << std::endl;
    }

    // Ahora sí hacer join (el thread debería terminar rápido)
    if (notification_thread.joinable()) {
        try {
            std::cout << "[LobbyClient] Joining listener thread..." << std::endl;
            notification_thread.join();
            std::cout << "[LobbyClient] Listener thread joined" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[LobbyClient] Error joining listener: " << e.what() << std::endl;
        }
    } else {
        std::cout << "[LobbyClient] No join needed (not joinable)" << std::endl;
    }
}

void LobbyClient::notification_listener() {
    std::cout << "[LobbyClient] Notification listener running..." << std::endl;

    try {
        while (listening.load() && connected) {
            uint8_t msg_type;

            try {
                msg_type = protocol.read_message_type();
            } catch (const std::exception& e) {
                if (!listening.load()) {
                    std::cout << "[LobbyClient] Listener stopped gracefully" << std::endl;
                    break;
                }
                throw;
            }

            // Si estamos cerrando, ignorar todos los mensajes
            if (!listening.load()) {
                std::cout << "[LobbyClient] Listener stopped, ignoring message type "
                          << static_cast<int>(msg_type) << std::endl;
                break;
            }

            std::cout << "[LobbyClient] Received notification type: " << static_cast<int>(msg_type)
                      << std::endl;

            switch (msg_type) {
            case MSG_PLAYER_JOINED_NOTIFICATION: {
                std::string user = protocol.read_string();
                std::cout << "[LobbyClient] Player joined: " << user << std::endl;
                emit playerJoinedNotification(QString::fromStdString(user));
                break;
            }

            case MSG_PLAYER_LEFT_NOTIFICATION: {
                std::string user = protocol.read_string();
                std::cout << "[LobbyClient] Player left: " << user << std::endl;
                emit playerLeftNotification(QString::fromStdString(user));
                break;
            }

            case MSG_PLAYER_READY_NOTIFICATION: {
                std::string user = protocol.read_string();
                uint8_t is_ready = protocol.read_uint8();
                std::cout << "[LobbyClient] Player " << user << " is now "
                          << (is_ready ? "READY" : "NOT READY") << std::endl;
                emit playerReadyNotification(QString::fromStdString(user), is_ready != 0);
                break;
            }

            case MSG_CAR_SELECTED_NOTIFICATION: {
                std::string user = protocol.read_string();
                std::string car_name = protocol.read_string();
                std::string car_type = protocol.read_string();
                std::cout << "[LobbyClient] Player " << user << " selected " << car_name
                          << std::endl;
                emit carSelectedNotification(QString::fromStdString(user),
                                             QString::fromStdString(car_name),
                                             QString::fromStdString(car_type));
                break;
            }

            case MSG_GAMES_LIST: {
                std::cout << "[LobbyClient] Received MSG_GAMES_LIST in listener (consuming fully)"
                          << std::endl;

                uint16_t count = protocol.read_uint16();
                std::cout << "[LobbyClient] Games list has " << count << " games" << std::endl;

                // Usar la función helper para evitar duplicación
                std::vector<GameInfo> games = protocol.read_games_list_from_socket(count);

                emit gamesListReceived(games);

                std::cout << "[LobbyClient] MSG_GAMES_LIST fully consumed, exiting listener"
                          << std::endl;

                // Salir del listener
                listening.store(false);
                return;
            }

            case MSG_ERROR: {
                uint8_t error_code = protocol.read_uint8();
                std::string error_msg = protocol.read_string();
                std::cerr << "[LobbyClient] Error " << static_cast<int>(error_code) << ": "
                          << error_msg << std::endl;
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

// esta
void LobbyClient::set_ready(bool is_ready) {
    protocol.set_ready(is_ready);
}

void LobbyClient::read_room_snapshot(std::vector<QString>& players,
                                     std::map<QString, QString>& cars) {
    std::cout << "[LobbyClient] Reading room snapshot manually..." << std::endl;

    while (true) {
        uint8_t msg_type;

        try {
            msg_type = protocol.read_message_type();
        } catch (const std::exception& e) {
            std::cerr << "[LobbyClient] Error reading snapshot: " << e.what() << std::endl;
            break;
        }

        std::cout << "[LobbyClient] Snapshot msg type: " << static_cast<int>(msg_type) << std::endl;

        if (msg_type == MSG_ROOM_SNAPSHOT) {
            uint8_t count1 = protocol.read_uint8();
            uint8_t count2 = protocol.read_uint8();

            if (count1 == 0 && count2 == 0) {
                std::cout << "[LobbyClient] ✅ END OF SNAPSHOT detected" << std::endl;
                break;  // Fin del snapshot
            }
        }

        // Procesar mensajes de snapshot
        if (msg_type == MSG_PLAYER_JOINED_NOTIFICATION) {
            std::string user = protocol.read_string();
            players.push_back(QString::fromStdString(user));
            std::cout << "[LobbyClient]   Snapshot player: " << user << std::endl;

        } else if (msg_type == MSG_CAR_SELECTED_NOTIFICATION) {
            std::string user = protocol.read_string();
            std::string car_name = protocol.read_string();
            std::string car_type = protocol.read_string();
            cars[QString::fromStdString(user)] = QString::fromStdString(car_name);
            std::cout << "[LobbyClient]   Snapshot car: " << user << " -> " << car_name << std::endl;

        } else if (msg_type == MSG_PLAYER_READY_NOTIFICATION) {
            std::string user = protocol.read_string();
            uint8_t is_ready = protocol.read_uint8();
            std::cout << "[LobbyClient]   Snapshot ready: " << user << " -> "
                      << (is_ready ? "YES" : "NO") << std::endl;
        }
    }

    std::cout << "[LobbyClient] Snapshot read complete: " << players.size() << " players"
              << std::endl;
}

// Destructor actualizado
LobbyClient::~LobbyClient() {
    // Asegurar join antes de destruir el objeto para evitar std::terminate
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
// //
// ========================================================================================================================================================================
