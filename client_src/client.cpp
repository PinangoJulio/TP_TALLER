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
    player_id = protocol.receive_client_id();
    receiver.set_id(player_id);
}

void Client::start() {
    try {

        LobbyController controller(this->protocol);
        QEventLoop lobbyLoop;
        
        QObject::connect(&controller, &LobbyController::lobbyFinished, &lobbyLoop, [&](bool success) {
            if (!success) {
                active = false;
            }
            lobbyLoop.quit();
        });

        controller.start();
        lobbyLoop.exec();

        if (!active) {
            return;
        }
        
        username = controller.getPlayerName().toStdString();
        races_paths = controller.getRacePaths();

        controller.closeAllWindows();
        QCoreApplication::processEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // ---------------------------------------------------------
        // FASE 2: COMUNICACIÓN

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
                game_renderer.init_race(races_paths[0]);
            }

            ClientEventHandler event_handler(command_queue, player_id, active);

            int ms_per_frame = 1000 / FPS;
            GameState current_snapshot;
            bool race_finished = false;
            bool ranking_phase = false;
            auto ranking_start = std::chrono::steady_clock::time_point{};
            size_t current_race_index = 0;


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

            // 1. Ordenar resultados por tiempo total acumulado
            std::sort(final_game_results.begin(), final_game_results.end(),
                [](const InfoPlayer& a, const InfoPlayer& b) {
                    // Ordenar por tiempo total acumulado (menor a mayor)
                    return a.total_time_ms < b.total_time_ms;
                });

            // 2. Convertir InfoPlayer (Lógica Juego) -> PlayerResult (Vista Qt)
            std::vector<PlayerResult> view_results;
            for (size_t i = 0; i < final_game_results.size(); ++i) {
                const auto& p = final_game_results[i];
                PlayerResult res;
                res.rank = (int)(i + 1);
                res.playerName = QString::fromStdString(p.username);
                res.carName = QString::fromStdString(p.car_name);

                // Formatear tiempo TOTAL: "MM:SS.ms"
                int min = p.total_time_ms / 60000;
                int sec = (p.total_time_ms % 60000) / 1000;
                int ms = p.total_time_ms % 1000;
                std::ostringstream timeStream;
                timeStream << std::setfill('0') << std::setw(2) << min << ":"
                           << std::setw(2) << sec << "."
                           << std::setw(3) << ms;
                res.totalTime = QString::fromStdString(timeStream.str());

                view_results.push_back(res);
            }

            // 3. Mostrar Ventana
            FinalRankingWindow rankingWindow;
            rankingWindow.setResults(view_results);

            rankingWindow.show();
            rankingWindow.raise();
            rankingWindow.activateWindow();

            // Procesar eventos pendientes de Qt para asegurar que la ventana se muestre
            QCoreApplication::processEvents();

            QEventLoop rankingLoop;
            QObject::connect(&rankingWindow, &FinalRankingWindow::returnToLobbyRequested, &rankingLoop, &QEventLoop::quit);

            // Loop bloqueante de Qt para mantener la ventana abierta
            rankingLoop.exec();
        }

    } catch (const std::runtime_error& e) {
        std::string error_msg = e.what();
        

        if (error_msg.find("Server shutdown") != std::string::npos) {

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


        
        sender.join();
        receiver.join();

    }

}