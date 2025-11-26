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

            // Log del comando que se va a enviar
            std::cout << "[ClientSender]  Enviando comando: ";
            switch (command.command) {
            case GameCommand::ACCELERATE:
                std::cout << "ACCELERATE";
                break;
            case GameCommand::BRAKE:
                std::cout << "BRAKE";
                break;
            case GameCommand::TURN_LEFT:
                std::cout << "TURN_LEFT";
                break;
            case GameCommand::TURN_RIGHT:
                std::cout << "TURN_RIGHT";
                break;
            case GameCommand::USE_NITRO:
                std::cout << "USE_NITRO";
                break;
            case GameCommand::STOP_ALL:
                std::cout << "STOP_ALL";
                break;
            case GameCommand::DISCONNECT:
                std::cout << "DISCONNECT";
                break;
            case GameCommand::CHEAT_INVINCIBLE:
                std::cout << "CHEAT_INVINCIBLE";
                break;
            case GameCommand::CHEAT_WIN_RACE:
                std::cout << "CHEAT_WIN_RACE";
                break;
            case GameCommand::CHEAT_LOSE_RACE:
                std::cout << "CHEAT_LOSE_RACE";
                break;
            case GameCommand::CHEAT_MAX_SPEED:
                std::cout << "CHEAT_MAX_SPEED";
                break;
            default:
                std::cout << "UNKNOWN(" << static_cast<int>(command.command) << ")";
                break;
            }
            std::cout << " (player_id=" << command.player_id << ")" << std::endl;

            protocol.send_command_client(command);

        } catch (const ClosedQueue& e) {
            std::cerr << "[ClientSender] ❌ Error sending command: " << e.what() << std::endl;
            break;
        }
    }

    std::cout << "[ClientSender]  Thread finalizado" << std::endl;
}

ClientSender::~ClientSender() {}
