#include "client_sender.h"

#include <iostream>

#include "client_protocol.h"

ClientSender::ClientSender(ClientProtocol& protocol, Queue<ComandMatchDTO>& cmd_queue)
    : protocol(protocol), commands_queue(cmd_queue) {}

void ClientSender::run() {

    while (should_keep_running()) {
        try {
            ComandMatchDTO command;
            command = commands_queue.pop();  // Bloquea hasta recibir un comando


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
}

ClientSender::~ClientSender() {}
