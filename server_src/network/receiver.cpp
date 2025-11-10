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


// ------------------------------------------------------
// Devuelve vector de (ciudad, [(yaml, png)])
// ------------------------------------------------------
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
            std::cout << "Ciudad: " << nombre_ciudad << ", carreras encontradas: " << carreras.size() << "\n";
        }
    }
    return ciudades_y_carreras;
}


void Receiver::handle_lobby() {
    while (is_running) {
        bool ok = protocol.handle_client();
        if (!ok) {
            std::cout << "[Receiver] Cliente desconectado. Finalizando hilo.\n";
            is_running = false;
            break;
        }
    }

    //sender.start();  --> cuando termina la parte del lobby comienza la comunicación multihilo
}

void Receiver::handle_match_messages() {
    //implementar comunicacion de juego
    is_running = false;
}


void Receiver::run() {
    handle_lobby();
    // handle_match_messages  --> llamar una vez que termina el lobby y arranca la comunicacion del juego
}

void Receiver::kill() { is_running = false; }

bool Receiver::status() { return is_running; }

Receiver::~Receiver() {}
