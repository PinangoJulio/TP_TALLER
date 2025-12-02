#include "client_receiver.h"

ClientReceiver::ClientReceiver(ClientProtocol& protocol, Queue<GameState>& queue)
    : protocol(protocol), snapshots_queue(queue), client_id(-1) {}

void ClientReceiver::run() {
    std::cout << "[ClientReceiver] Thread iniciado, esperando mensajes del servidor..." << std::endl;

    while (should_keep_running()) {
        try {
            // 1. LEER TIPO DE MENSAJE
            uint8_t msg_type;
            try {
                msg_type = protocol.read_message_type();
            } catch (const std::exception& e) {
                if (should_keep_running()) {
                    std::cout << "[ClientReceiver] Conexión cerrada o error al leer tipo: " << e.what() << std::endl;
                }
                break;
            }

            // 2. PROCESAR SEGÚN TIPO
            // Usamos switch con casting a int para poder manejar los mensajes del Lobby (32, 34, 35)
            switch (static_cast<int>(msg_type)) {
                
                // --- MENSAJES DEL JUEGO (Lo normal) ---
                case (int)ServerMessageType::GAME_STATE_UPDATE: {
                    // receive_snapshot NO debe leer el tipo, ya lo leímos arriba
                    GameState game_state_snapshot = protocol.receive_snapshot();

                    if (game_state_snapshot.players.empty()) continue;

                    if (game_state_snapshot.race_info.status == MatchStatus::FINISHED) {
                        std::cout << "[ClientReceiver] MatchStatus marcado como FINISHED." << std::endl;
                        this->stop();
                    }

                    InfoPlayer* player = game_state_snapshot.findPlayer(client_id);
                    if (player && player->disconnected) {
                         std::cout << "[ClientReceiver] Jugador local desconectado." << std::endl;
                        this->stop();
                    }

                    snapshots_queue.try_push(game_state_snapshot);
                    break;
                }

                case (int)ServerMessageType::RACE_FINISHED: {
                    std::cout << "[ClientReceiver] Recibido mensaje de FIN DE CARRERA." << std::endl;
                    this->stop();
                    break;
                }

                // --- LIMPIEZA DE MENSAJES DEL LOBBY ("Resaca") ---
                // Estos mensajes llegaron tarde cuando ya estábamos entrando al juego.
                // Los leemos para vaciar el socket.

                case 32: // MSG_PLAYER_JOINED_NOTIFICATION
                    {
                        std::string trash_user = protocol.read_string();
                        std::cout << "[ClientReceiver] 🧹 Limpiando mensaje Lobby (Joined): " << trash_user << std::endl;
                    }
                    break;

                case 34: // MSG_PLAYER_READY_NOTIFICATION
                    {
                        std::string trash_user = protocol.read_string();
                        // Leemos el bool pero NO lo guardamos en variable para evitar error de compilación
                        protocol.read_uint8(); 
                        
                        std::cout << "[ClientReceiver] 🧹 Limpiando mensaje Lobby (Ready): " << trash_user << std::endl;
                    }
                    break;

                case 35: // MSG_CAR_SELECTED_NOTIFICATION
                    {
                        // Estructura: User(str) + Car(str) + Type(str)
                        std::string trash_user = protocol.read_string();
                        std::string trash_car = protocol.read_string();
                        // Leemos el type pero NO lo guardamos para evitar error de compilación
                        protocol.read_string(); 

                        std::cout << "[ClientReceiver] 🧹 Limpiando mensaje Lobby (Car): " << trash_user << " - " << trash_car << std::endl;
                    }
                    break;

                default: {
                    std::cerr << "[ClientReceiver] ⚠️ Tipo de mensaje desconocido: " 
                              << static_cast<int>(msg_type) << ". Posible desincronización." << std::endl;
                    break;
                }
            }

        } catch (const ClosedQueue&) {
            std::cout << "[ClientReceiver] Queue cerrada, terminando." << std::endl;
            break;
        } catch (const std::exception& e) {
            std::cerr << "[ClientReceiver] ❌ Error crítico en loop: " << e.what() << std::endl;
            break;
        }
    }

    std::cout << "[ClientReceiver] Thread finalizado" << std::endl;
}

ClientReceiver::~ClientReceiver() {
    stop();
    join();
}