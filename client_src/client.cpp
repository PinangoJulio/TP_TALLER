#include "client.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QObject>
#include <SDL2pp/SDL2pp.hh>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>
#include <iomanip>
#include <sstream>

#include "client_event_handler.h"
#include "lobby/controller/lobby_controller.h"
#include "game/game_renderer.h"
// Asegúrate que esta ruta sea correcta (mayúsculas/minúsculas)
#include "lobby/Rankings/final_ranking.h" 

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

        // Variables para guardar los resultados y mostrarlos después de cerrar SDL
        std::vector<InfoPlayer> final_game_results;
        bool show_qt_ranking = false;

        // ---------------------------------------------------------
        // FASE 3: SDL Y JUEGO (ENCAPSULADO EN BLOQUE)
        // ---------------------------------------------------------
        // Usamos este bloque {} para que Window y Renderer se destruyan automáticamente
        // cuando salgamos del loop, liberando SDL antes de abrir Qt de nuevo.
        {
            SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
            if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
                std::cerr << "Error SDL_image: " << IMG_GetError() << std::endl;
            }
            if (TTF_Init() == -1) {
                std::cerr << "Error SDL_ttf: " << TTF_GetError() << std::endl;
            }

            Window window(NFS_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                        GameRenderer::SCREEN_WIDTH, GameRenderer::SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
            
            Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);
            GameRenderer game_renderer(renderer);
            
            if (!races_paths.empty()) {
                std::cout << "[Client] Cargando primera carrera: " << races_paths[0] << std::endl;
                game_renderer.init_race(races_paths[0]);
            }

            ClientEventHandler event_handler(command_queue, player_id, active);

            int ms_per_frame = 1000 / FPS;
            GameState current_snapshot; 
            bool race_finished = false;
            bool ranking_phase = false;
            auto ranking_start = std::chrono::steady_clock::time_point{};
            size_t current_race_index = 0;

            std::cout << "[Client] Entrando al game loop..." << std::endl;

            while (active) {
                auto t1 = std::chrono::steady_clock::now();

                // 1. Consumir snapshots
                GameState new_snapshot;
                while (snapshot_queue.try_pop(new_snapshot)) {
                    current_snapshot = new_snapshot;
                }

                bool all_finished = true;
                int vivos = 0;
                for (const auto& p : current_snapshot.players) {
                    if (!p.is_alive) continue; 
                    vivos++;
                    if (!p.race_finished) { all_finished = false; }
                }

                if (!ranking_phase) {
                    event_handler.handle_events();
                }

                game_renderer.render(current_snapshot, player_id);

                if (all_finished && vivos > 0 && !ranking_phase && !race_finished) {
                    race_finished = true;
                    ranking_phase = true;
                    ranking_start = std::chrono::steady_clock::now();
                    std::cout << "[Client] Todos terminaron. Ranking temporal " << RANKING_SECONDS << "s..." << std::endl;
                }

                if (ranking_phase) {
                    auto elapsed_rank = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - ranking_start).count();
                    if (elapsed_rank >= RANKING_SECONDS) {
                        ranking_phase = false;
                        race_finished = false;
                        if (current_race_index + 1 < races_paths.size()) {
                            current_race_index++;
                            std::cout << "[Client] Siguiente carrera: " << races_paths[current_race_index] << std::endl;
                            game_renderer.init_race(races_paths[current_race_index]);
                        } else {
                            // ========================================
                            // FIN DEL JUEGO: PREPARAR DATOS PARA QT
                            // ========================================
                            std::cout << "[Client] Carreras terminadas. Cambiando a vista de Ranking Qt..." << std::endl;
                            
                            final_game_results = current_snapshot.players;
                            show_qt_ranking = true;
                            active = false; // Salir del loop SDL
                        }
                    }
                }

                auto t2 = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
                if (elapsed < ms_per_frame) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
                }
            }
            // AQUÍ MUERE LA VENTANA SDL AUTOMÁTICAMENTE (RAII)
        } 
        
        // ---------------------------------------------------------
        // FASE 4: RANKING FINAL (QT)
        // ---------------------------------------------------------
        if (show_qt_ranking) {
            std::cout << "[Client] Lanzando ventana de Ranking Qt..." << std::endl;

            // 1. Ordenar resultados
            std::sort(final_game_results.begin(), final_game_results.end(),
                [](const InfoPlayer& a, const InfoPlayer& b) {
                    if (a.race_finished && b.race_finished) {
                        return a.race_time_ms < b.race_time_ms;
                    }
                    if (a.race_finished) return true;
                    if (b.race_finished) return false;
                    return a.position_in_race < b.position_in_race;
                });

            // 2. Convertir InfoPlayer (Lógica Juego) -> PlayerResult (Vista Qt)
            std::vector<PlayerResult> view_results;
            for (size_t i = 0; i < final_game_results.size(); ++i) {
                const auto& p = final_game_results[i];
                PlayerResult res;
                res.rank = (int)(i + 1);
                res.playerName = QString::fromStdString(p.username);
                res.carName = QString::fromStdString(p.car_name);
                
                // Formatear tiempo: "MM:SS.ms" o "DNF"
                if (p.race_finished) {
                    int min = p.race_time_ms / 60000;
                    int sec = (p.race_time_ms % 60000) / 1000;
                    int ms = p.race_time_ms % 1000;
                    std::ostringstream timeStream;
                    timeStream << std::setfill('0') << std::setw(2) << min << ":"
                               << std::setw(2) << sec << "."
                               << std::setw(3) << ms;
                    res.totalTime = QString::fromStdString(timeStream.str());
                } else {
                    res.totalTime = "DNF";
                }
                
                view_results.push_back(res);
            }

            // 3. Mostrar Ventana
            FinalRankingWindow rankingWindow;
            rankingWindow.setResults(view_results);
            rankingWindow.show();

            QEventLoop rankingLoop;
            QObject::connect(&rankingWindow, &FinalRankingWindow::returnToLobbyRequested, &rankingLoop, &QEventLoop::quit);
            
            // Loop bloqueante de Qt para mantener la ventana abierta
            rankingLoop.exec();
            
            std::cout << "[Client] Ventana de ranking cerrada." << std::endl;
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "[Client] Error: " << e.what() << std::endl;
        active = false;
        SDL_Quit();
    }
}

Client::~Client() {
    std::cout << "[Client] Destructor llamado" << std::endl;

    if (threads_started) {
        sender.stop();
        receiver.stop();
        try { command_queue.close(); } catch (...) {}
        try { snapshot_queue.close(); } catch (...) {}
        sender.join();
        receiver.join();
    }
}