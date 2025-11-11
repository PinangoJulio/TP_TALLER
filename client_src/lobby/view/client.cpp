#include "client.h"

#include <algorithm>
#include <iostream>

#include "../common_src/utils.h"

// pasa por stdin nitro, read n, exit

Client::Client(const std::string& host, const std::string& port): protocol_c(host, port) {}

void Client::read_n_messages(const int& messages) {
    for (int i = 0; i < messages; i++) {
        ServerMsg msg = this->protocol_c.read_msg();
        if (msg.nitro_status == CodeActions::NITRO_ON) {
            std::cout << "A car hit the nitro!" << std::endl;
        } else if (msg.nitro_status == CodeActions::NITRO_OFF) {
            std::cout << "A car is out of juice." << std::endl;
        }
    }
}

void Client::run() {
    std::string command;

    while (std::cin >> command) {
        if (command == "nitro") {
            this->protocol_c.send_msg_nitro();

        } else if (command == "read") {
            int messages;
            std::cin >> messages;
            read_n_messages(messages);

        } else if (command == "exit") {
            break;
        }
    }
}
