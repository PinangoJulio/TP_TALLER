#include "client_sender.h"

#include <iostream>

#include "client_protocol.h"

ClientSender::ClientSender(ClientProtocol& protocol, Queue<ComandMatchDTO>& cmd_queue)
    : protocol(protocol), commands_queue(cmd_queue) {}

void ClientSender::run() {
    std::cout << "[ClientSender]  Thread iniciado, esperando comandos..." << std::endl;

    while (should_keep_running()) {
        try {
            ComandMatchDTO command;
            command = commands_queue.pop();  // Bloquea hasta recibir un comando

            // Log solo para comandos importantes (no movimiento básico)
            if (command.command == GameCommand::DISCONNECT ||
                command.command == GameCommand::CHEAT_WIN_RACE ||
                command.command == GameCommand::CHEAT_LOSE_RACE ||
                command.command == GameCommand::CHEAT_INVINCIBLE) {
                std::cout << "[ClientSender]  Comando importante: ";
                switch (command.command) {
                case GameCommand::DISCONNECT:
                    std::cout << "DISCONNECT";
                    break;
                case GameCommand::CHEAT_WIN_RACE:
                    std::cout << "CHEAT_WIN_RACE";
                    break;
                case GameCommand::CHEAT_LOSE_RACE:
                    std::cout << "CHEAT_LOSE_RACE";
                    break;
                case GameCommand::CHEAT_INVINCIBLE:
                    std::cout << "CHEAT_INVINCIBLE";
                    break;
                default:
                    break;
                }
                std::cout << " (player_id=" << command.player_id << ")" << std::endl;
            }

            protocol.send_command_client(command);

        } catch (const ClosedQueue& e) {
            // Cola cerrada, salir limpiamente sin reportar error
            break;
        } catch (const std::exception& e) {
            // Solo reportar error si no estamos cerrando intencionalmente
            if (should_keep_running()) {
                std::cerr << "[ClientSender]  Error enviando comando: " << e.what() << std::endl;
            }
            break;
        }
    }

    std::cout << "[ClientSender]  Thread finalizado" << std::endl;
}

ClientSender::~ClientSender() {}
