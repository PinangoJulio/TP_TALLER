#include "client.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QObject>
#include <SDL2pp/SDL2pp.hh>
#include <chrono>
#include <iostream>
#include <thread>

#include "client_event_handler.h"
#include "lobby/controller/lobby_controller.h"
#include "game/game_renderer.h"

#define NFS_TITLE      "Need for Speed 2D"
#define FPS            60

using namespace SDL2pp;

Client::Client(const char* hostname, const char* servname)
    : protocol(hostname, servname), username("Player"),
      player_id(-1), races_paths() , active(true), command_queue(), snapshot_queue(),
      sender(protocol, command_queue), receiver(protocol, snapshot_queue), threads_started(false) {
    std::cout << "[Client] Cliente inicializado para " << username << std::endl;
    player_id = protocol.receive_client_id();
    receiver.set_id(player_id);
}

void Client::start() {
    // ---------------------------------------------------------
    // FASE 1: LOBBY (QT)
    // ---------------------------------------------------------
    std::cout << "[Client] Iniciando fase de lobby Qt..." << std::endl;

    LobbyController controller(this->protocol);
    QEventLoop lobbyLoop;
    
    QObject::connect(&controller, &LobbyController::lobbyFinished, &lobbyLoop, [&](bool success) {
        std::cout << "[Client] Lobby terminado (success=" << success << ")" << std::endl;
        if (!success) {
            active = false;
        }
        lobbyLoop.quit();
    });

    controller.start();
    lobbyLoop.exec(); // Bloquea hasta fin del lobby

    if (!active) {
        std::cout << "[Client] Abortando inicio de juego." << std::endl;
        return;
    }
    
    username = controller.getPlayerName().toStdString();
    races_paths = controller.getRacePaths();

    controller.closeAllWindows();
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ---------------------------------------------------------
    // FASE 2: COMUNICACIÓN
    // ---------------------------------------------------------
    std::cout << "[Client] Iniciando threads de comunicación..." << std::endl;
    sender.start();
    receiver.start();
    threads_started = true;

    // ---------------------------------------------------------
    // FASE 3: SDL Y JUEGO
    // ---------------------------------------------------------
    SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Error SDL_image: " << IMG_GetError() << std::endl;
    }

    // ✅ CORRECCIÓN: Usar constantes del Renderer para el tamaño de ventana
    Window window(NFS_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                  GameRenderer::SCREEN_WIDTH, GameRenderer::SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    
    Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    // ✅ CORRECCIÓN: Constructor simplificado (sin argumentos de tamaño)
    GameRenderer game_renderer(renderer);
    
    // Cargar la primera carrera si existe
    if (!races_paths.empty()) {
        std::cout << "[Client] Cargando primera carrera: " << races_paths[0] << std::endl;
        game_renderer.init_race(races_paths[0]);
    } else {
        std::cerr << "[Client] ⚠️ ALERTA: No se recibieron rutas de carreras!" << std::endl;
    }

    ClientEventHandler event_handler(command_queue, player_id, active);

    // ---------------------------------------------------------
    // FASE 4: GAME LOOP
    // ---------------------------------------------------------
    int ms_per_frame = 1000 / FPS;
    GameState current_snapshot; 
    bool race_finished = false;

    std::cout << "[Client] Entrando al game loop..." << std::endl;

    while (active) {
        auto t1 = std::chrono::steady_clock::now();

        // 1. Red
        GameState new_snapshot;
        while (snapshot_queue.try_pop(new_snapshot)) {
            current_snapshot = new_snapshot;
            InfoPlayer* local = current_snapshot.findPlayer(player_id);
            if (local && local->race_finished) {
                race_finished = true;
            }
        }
    
        // 2. Input
        event_handler.handle_events();

        // 3. Render
        game_renderer.render(current_snapshot, player_id);

        // 4. Timing
        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        if (elapsed < ms_per_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
        }

        if (race_finished) {
            std::cout << "[Client] Carrera terminada. Saliendo..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            active = false;
        }
    }
}

Client::~Client() {
    std::cout << "[Client] Destructor llamado" << std::endl;

    if (threads_started) {
        std::cout << "[Client] Cerrando threads de comunicación..." << std::endl;

        // 1. Señalizar a los threads que deben detenerse
        sender.stop();
        receiver.stop();

        // 2. Cerrar las colas para desbloquear los threads si están esperando
        command_queue.close();
        snapshot_queue.close();

        // 3. Esperar a que los threads finalicen
        sender.join();
        receiver.join();

        std::cout << "[Client] Threads finalizados correctamente" << std::endl;
    } else {
        std::cout << "[Client]  Threads ya fueron cerrados previamente" << std::endl;
    }

    std::cout << "[Client]  Destructor completado" << std::endl;
}