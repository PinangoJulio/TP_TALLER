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
#define RANKING_SECONDS 5

using namespace SDL2pp;

Client::Client(const char* hostname, const char* servname)
    : protocol(hostname, servname), username("Player"),
      player_id(-1), races_paths(), active(true), command_queue(), snapshot_queue(),
      sender(protocol, command_queue), receiver(protocol, snapshot_queue), threads_started(false) {
    std::cout << "[Client] Cliente inicializado para " << username << std::endl;
    player_id = protocol.receive_client_id();
    receiver.set_id(player_id);
}

void Client::start() {
    try {
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
        lobbyLoop.exec();

        
        if (!active) {
            std::cout << "[Client] Abortando inicio de juego (server shutdown o error)." << std::endl;
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

        Window window(NFS_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                      GameRenderer::SCREEN_WIDTH, GameRenderer::SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
        
        Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);
        GameRenderer game_renderer(renderer);
        
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
        bool ranking_phase = false;
        auto ranking_start = std::chrono::steady_clock::time_point{};
        size_t current_race_index = 0;

        std::cout << "[Client] Entrando al game loop..." << std::endl;

        while (active) {
            auto t1 = std::chrono::steady_clock::now();

            // 1. Consumir snapshots y evaluar estado global
            GameState new_snapshot;
            while (snapshot_queue.try_pop(new_snapshot)) {
                current_snapshot = new_snapshot;
            }

            // Verificar si todos terminaron (ignorar desconectados/no vivos)
            bool all_finished = true;
            int vivos = 0;
            for (const auto& p : current_snapshot.players) {
                if (!p.is_alive) continue; // tratar desconectados como no requeridos
                vivos++;
                if (!p.race_finished) { all_finished = false; }
            }

            // 2. Entrada solo si no estamos en ranking
            if (!ranking_phase) {
                event_handler.handle_events();
            }

            // 3. Render normal (podría agregar overlay de ranking si ranking_phase)
            game_renderer.render(current_snapshot, player_id);

            // 4. Activar ranking solo cuando TODOS terminaron
            if (all_finished && vivos > 0 && !ranking_phase && !race_finished) {
                race_finished = true;      // carrera global finalizada
                ranking_phase = true;      // entrar a ranking
                ranking_start = std::chrono::steady_clock::now();
                std::cout << "[Client] Todos los jugadores terminaron. Mostrando ranking " << RANKING_SECONDS << "s..." << std::endl;
            }

            // 5. Permanecer en ranking y luego iniciar siguiente carrera
            if (ranking_phase) {
                auto elapsed_rank = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - ranking_start).count();
                if (elapsed_rank >= RANKING_SECONDS) {
                    ranking_phase = false;
                    race_finished = false;
                    if (current_race_index + 1 < races_paths.size()) {
                        current_race_index++;
                        std::cout << "[Client] Iniciando siguiente carrera tras ranking: " << races_paths[current_race_index] << std::endl;
                        game_renderer.init_race(races_paths[current_race_index]);
                    } else {
                        std::cout << "[Client] Partida completa. Cerrando cliente." << std::endl;
                        active = false;
                    }
                }
            }

            // 6. Timing
            auto t2 = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
            if (elapsed < ms_per_frame) {
                std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
            }
        }
        
    } catch (const std::runtime_error& e) {
        std::string error_msg = e.what();
        
        
        if (error_msg.find("Server shutdown") != std::string::npos) {
            std::cout << "\n==================================================" << std::endl;
            std::cout << "    ⚠️  SERVER CERRADO - SALIENDO DEL JUEGO" << std::endl;
            std::cout << "==================================================" << std::endl;
            
            active = false;
            
            // Si estamos en SDL, cerrarlo
            SDL_Quit();
            
        } else {
            std::cerr << "[Client] Error durante ejecución: " << error_msg << std::endl;
            active = false;
        }
    }
}


Client::~Client() {
    std::cout << "[Client] Destructor llamado" << std::endl;

    if (threads_started) {
        std::cout << "[Client] Cerrando threads de comunicación..." << std::endl;

        sender.stop();
        receiver.stop();

        
        try {
            command_queue.close();
        } catch (...) {}

        try {
            snapshot_queue.close();
        } catch (...) {}

        
        auto wait_start = std::chrono::steady_clock::now();
        const int TIMEOUT_SECONDS = 5;
        
        while (receiver.is_alive() && 
               std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - wait_start).count() < TIMEOUT_SECONDS) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (receiver.is_alive()) {
            std::cout << "[Client] ⚠️ Receiver timeout - forcing shutdown" << std::endl;
        }
        
        sender.join();
        receiver.join();

        std::cout << "[Client]   Threads finalizados" << std::endl;
    }

    std::cout << "[Client] Destructor completado" << std::endl;
}