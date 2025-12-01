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
#define DEFAULT_WIDTH  700
#define DEFAULT_HEIGHT 700
#define FPS            60

using namespace SDL2pp;

Client::Client(const char* hostname, const char* servname)
    : protocol(hostname, servname), username("Player"),  // Obtener del lobby Qt
      player_id(-1), active(true), command_queue(), snapshot_queue(),
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

    // Event loop temporal para esperar fin del lobby
    QEventLoop lobbyLoop;
    
    QObject::connect(&controller, &LobbyController::lobbyFinished, &lobbyLoop, 
        [&](bool success) {
            std::cout << "[Client] Lobby terminado (success=" << success << ")" << std::endl;
            if (!success) {
                active = false;  // abortar cliente si lobby falla
            }
            lobbyLoop.quit();
        });

    controller.start();
    lobbyLoop.exec();  // ✅ Bloquea hasta que el lobby emite lobbyFinished

    if (!active) {
        std::cout << "[Client] Abortando inicio de juego por fallo en lobby" << std::endl;
        return;
    }
    
    username = controller.getPlayerName().toStdString();
    std::cout << "[Client] Usuario listo: " << username << std::endl;

    // ✅ Cerrar ventanas Qt DESPUÉS de que el QEventLoop termine
    std::cout << "[Client] Cerrando ventanas Qt..." << std::endl;
    controller.closeAllWindows();

    // Procesar eventos pendientes de Qt para que se cierren las ventanas
    QCoreApplication::processEvents();

    // Pequeña espera para asegurar que Qt termine de cerrar las ventanas
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ✅ LOBBY COMPLETADO EXITOSAMENTE                        ║" << std::endl;
    std::cout << "║  🎮 Iniciando fase de juego SDL...                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";

    // ---------------------------------------------------------
    // FASE 2: INICIAR THREADS DE COMUNICACIÓN
    // ---------------------------------------------------------
    std::cout << "[Client] Iniciando threads de comunicación..." << std::endl;

    sender.start();
    receiver.start();
    threads_started = true;

    // ---------------------------------------------------------
    // FASE 3: SDL Y RECURSOS
    // ---------------------------------------------------------
    SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Error SDL_image: " << IMG_GetError() << std::endl;
    }

    Window window(NFS_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                  DEFAULT_WIDTH, DEFAULT_HEIGHT, SDL_WINDOW_SHOWN);
    
    Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    GameRenderer game_renderer(renderer, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    
    ClientEventHandler event_handler(command_queue, player_id, active);

    // ---------------------------------------------------------
    // FASE 4: GAME LOOP
    // ---------------------------------------------------------
    int ms_per_frame = 1000 / FPS;
    
    GameState current_snapshot; 
    bool race_finished = false;

    std::cout << "\n";
    std::cout << "[Client] 🏁 Entrando al game loop..." << std::endl;
    std::cout << "[Client]  Controles:" << std::endl;
    std::cout << "[Client]    W: Acelerar" << std::endl;
    std::cout << "[Client]    S: Frenar" << std::endl;
    std::cout << "[Client]    A: Girar izquierda" << std::endl;
    std::cout << "[Client]    D: Girar derecha" << std::endl;
    std::cout << "[Client]    ESPACIO: Usar nitro" << std::endl;
    std::cout << "[Client]    ESC: Salir del juego" << std::endl;
    std::cout << "[Client]" << std::endl;
    std::cout << "[Client]  Cheats:" << std::endl;
    std::cout << "[Client]    F1: Ganar carrera automáticamente" << std::endl;
    std::cout << "[Client]    P: Perder carrera" << std::endl;
    std::cout << "\n";

    // ---------------------------------------------------------
    // FASE 5: GAME LOOP PRINCIPAL
    // ---------------------------------------------------------
    while (active) {
        auto t1 = std::chrono::steady_clock::now();

        // 1. PROCESAR MENSAJES DE RED
        GameState new_snapshot;
        while (snapshot_queue.try_pop(new_snapshot)) {
            current_snapshot = new_snapshot;

            // Verificar si yo terminé
            InfoPlayer* local = current_snapshot.findPlayer(player_id);
            if (local && local->race_finished) {
                race_finished = true;
                std::cout << "[Client] ¡Carrera terminada!" << std::endl;
            }
        }
    
        // 2. INPUT
        event_handler.handle_events();

        // 3. RENDERIZADO
        game_renderer.render(current_snapshot, player_id);

        // 4. CONTROL DE FPS
        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
        if (elapsed < ms_per_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
        }

        // 5. VERIFICAR FIN DE CARRERA
        if (race_finished) {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            active = false;
        }
    }

    std::cout << "\n";
    std::cout << "[Client] Game loop finalizado" << std::endl;
    std::cout << "[Client]  Cerrando recursos..." << std::endl;

    // Cerrar threads de comunicación
    if (threads_started) {
        std::cout << "[Client]   → Deteniendo thread sender..." << std::endl;
        sender.stop();
        receiver.stop();

        std::cout << "[Client]   → Cerrando colas de comunicación..." << std::endl;
        command_queue.close();
        snapshot_queue.close();

        std::cout << "[Client]   → Esperando finalización de threads..." << std::endl;
        sender.join();
        receiver.join();

        std::cout << "[Client]   ✅ Threads finalizados correctamente" << std::endl;
        threads_started = false;
    }

    std::cout << "[Client] ✅ Todos los recursos cerrados correctamente" << std::endl;
}

Client::~Client() {
    std::cout << "[Client] Destructor llamado" << std::endl;

    // Solo cerrar threads si no fueron cerrados en start()
    if (threads_started) {
        std::cout << "[Client] ℹ️  Threads aún activos, cerrando desde destructor..." << std::endl;

        try {
            sender.stop();
            receiver.stop();
            command_queue.close();
            snapshot_queue.close();
            sender.join();
            receiver.join();

            std::cout << "[Client] ✅ Threads finalizados desde destructor" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Client] ⚠️  Error en destructor: " << e.what() << std::endl;
    }
    } else {
        std::cout << "[Client] ℹ️  Threads ya fueron cerrados previamente" << std::endl;
    }

    std::cout << "[Client] 👋 Destructor completado" << std::endl;
}