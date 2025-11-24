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
            std::cerr << "[ClientSender] Error sending command: " << e.what() << std::endl;
            break;
        }
    }
}

ClientSender::~ClientSender() {}
