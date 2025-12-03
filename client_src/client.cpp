#include "client.h"

#include <SDL2/SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

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

        // Inicializar SDL_ttf
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
                        // ========================================
                        // MOSTRAR PANTALLA DE RANKINGS FINAL
                        // ========================================
                        std::cout << "[Client] ¡Todas las carreras completadas!" << std::endl;
                        std::cout << "[Client] Mostrando rankings finales..." << std::endl;

                        // Ordenar jugadores por tiempo total (race_time_ms)
                        std::vector<InfoPlayer> final_rankings = current_snapshot.players;

                        // DEBUG: Mostrar datos antes de ordenar
                        std::cout << "\n========== DATOS DE RANKING ==========" << std::endl;
                        std::cout << "Total jugadores: " << final_rankings.size() << std::endl;
                        for (size_t i = 0; i < final_rankings.size(); i++) {
                            const auto& p = final_rankings[i];
                            std::cout << "Player " << i << ":" << std::endl;
                            std::cout << "  ID: " << p.player_id << std::endl;
                            std::cout << "  Username: '" << p.username << "'" << std::endl;
                            std::cout << "  Car: " << p.car_name << std::endl;
                            std::cout << "  Position: " << p.position_in_race << std::endl;
                            std::cout << "  Time (ms): " << p.race_time_ms << std::endl;
                            std::cout << "  Finished: " << (p.race_finished ? "YES" : "NO") << std::endl;
                            std::cout << "  Alive: " << (p.is_alive ? "YES" : "NO") << std::endl;
                        }
                        std::cout << "======================================\n" << std::endl;

                        std::sort(final_rankings.begin(), final_rankings.end(),
                            [](const InfoPlayer& a, const InfoPlayer& b) {
                                // Los que terminaron primero por tiempo
                                if (a.race_finished && b.race_finished) {
                                    return a.race_time_ms < b.race_time_ms;
                                }
                                if (a.race_finished) return true;
                                if (b.race_finished) return false;
                                return a.position_in_race < b.position_in_race;
                            });

                        // DEBUG: Mostrar orden final
                        std::cout << "\n========== ORDEN FINAL ==========" << std::endl;
                        for (size_t i = 0; i < final_rankings.size(); i++) {
                            std::cout << (i+1) << ". " << final_rankings[i].username
                                      << " - " << final_rankings[i].race_time_ms << "ms" << std::endl;
                        }
                        std::cout << "=================================\n" << std::endl;

                        // Cargar fuente para el ranking
                        TTF_Font* title_font = TTF_OpenFont("assets/fonts/arcade.ttf", 48);
                        TTF_Font* text_font = TTF_OpenFont("assets/fonts/arcade.ttf", 24);
                        TTF_Font* small_font = TTF_OpenFont("assets/fonts/arcade.ttf", 18);

                        if (!title_font || !text_font || !small_font) {
                            std::cerr << "[Client] Error cargando fuentes: " << TTF_GetError() << std::endl;
                            // Fallback: continuar sin fuentes
                        }

                        // Helper para renderizar texto
                        auto render_text = [&](TTF_Font* font, const std::string& text, int x, int y, SDL_Color color) {
                            if (!font) return;

                            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
                            if (!surface) return;

                            SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer.Get(), surface);
                            if (texture) {
                                SDL_Rect dest = {x, y, surface->w, surface->h};
                                SDL_RenderCopy(renderer.Get(), texture, nullptr, &dest);
                                SDL_DestroyTexture(texture);
                            }
                            SDL_FreeSurface(surface);
                        };

                        // Loop de renderizado del ranking final
                        bool exit_rankings = false;
                        while (!exit_rankings) {
                            SDL_Event event;
                            while (SDL_PollEvent(&event)) {
                                if (event.type == SDL_QUIT ||
                                    (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
                                    exit_rankings = true;
                                    active = false;
                                    break;
                                }
                            }

                            // Fondo negro
                            renderer.SetDrawColor(0, 0, 0, 255);
                            renderer.Clear();

                            // Banner título (dorado/amarillo)
                            int title_height = 100;
                            renderer.SetDrawColor(218, 165, 32, 255); // Dorado
                            renderer.FillRect(SDL2pp::Rect(0, 0, GameRenderer::SCREEN_WIDTH, title_height));

                            // Borde del título
                            renderer.SetDrawColor(255, 215, 0, 255);
                            for (int i = 0; i < 4; i++) {
                                renderer.DrawRect(SDL2pp::Rect(i, i, GameRenderer::SCREEN_WIDTH - i*2, title_height - i*2));
                            }

                            // Título con texto real
                            SDL_Color white = {255, 255, 255, 255};
                            render_text(title_font, "RANKINGS FINALES", 180, 25, white);

                            // Panel de rankings con información real
                            int panel_y = title_height + 50;
                            int row_height = 70;
                            int max_visible = 6;

                            // Encabezados de columnas
                            renderer.SetDrawColor(100, 100, 100, 255);
                            renderer.FillRect(SDL2pp::Rect(50, panel_y - 40, GameRenderer::SCREEN_WIDTH - 100, 35));

                            SDL_Color gray = {200, 200, 200, 255};
                            render_text(small_font, "POS", 70, panel_y - 35, gray);
                            render_text(small_font, "JUGADOR", 180, panel_y - 35, gray);
                            render_text(small_font, "TIEMPO", GameRenderer::SCREEN_WIDTH - 280, panel_y - 35, gray);
                            render_text(small_font, "ESTADO", GameRenderer::SCREEN_WIDTH - 140, panel_y - 35, gray);

                            for (size_t i = 0; i < final_rankings.size() && i < (size_t)max_visible; i++) {
                                const auto& player = final_rankings[i];
                                int y = panel_y + i * row_height;

                                // Fondo de la fila (alternar colores)
                                if (i % 2 == 0) {
                                    renderer.SetDrawColor(50, 50, 50, 255);
                                } else {
                                    renderer.SetDrawColor(40, 40, 40, 255);
                                }
                                renderer.FillRect(SDL2pp::Rect(50, y, GameRenderer::SCREEN_WIDTH - 100, row_height - 5));

                                // Borde izquierdo según posición
                                SDL_Color medal_color;
                                if (i == 0) {
                                    renderer.SetDrawColor(255, 215, 0, 255); // Oro
                                    medal_color = {255, 215, 0, 255};
                                } else if (i == 1) {
                                    renderer.SetDrawColor(192, 192, 192, 255); // Plata
                                    medal_color = {192, 192, 192, 255};
                                } else if (i == 2) {
                                    renderer.SetDrawColor(205, 127, 50, 255); // Bronce
                                    medal_color = {205, 127, 50, 255};
                                } else {
                                    renderer.SetDrawColor(100, 150, 200, 255); // Azul claro
                                    medal_color = {100, 150, 200, 255};
                                }
                                renderer.FillRect(SDL2pp::Rect(50, y, 10, row_height - 5));

                                // Número de posición con texto real
                                std::string pos_text = std::to_string(i + 1);
                                render_text(text_font, pos_text, 75, y + 20, medal_color);

                                // Nombre del jugador con texto real
                                std::string display_name = player.username;
                                if (display_name.length() > 15) {
                                    display_name = display_name.substr(0, 15) + "...";
                                }
                                render_text(text_font, display_name, 150, y + 20, white);

                                // Tiempo con texto real - Convertir ms a formato legible
                                SDL_Color green = {100, 255, 100, 255};

                                int total_ms = player.race_time_ms;
                                int minutes = total_ms / 60000;
                                int seconds = (total_ms % 60000) / 1000;
                                int ms = total_ms % 1000;

                                char time_buffer[32];
                                snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d.%03d", minutes, seconds, ms);
                                render_text(text_font, time_buffer, GameRenderer::SCREEN_WIDTH - 280, y + 20, green);

                                // Estado con texto real
                                if (player.race_finished) {
                                    SDL_Color finish_green = {0, 255, 0, 255};
                                    render_text(small_font, "OK", GameRenderer::SCREEN_WIDTH - 130, y + 25, finish_green);
                                } else {
                                    SDL_Color red = {255, 100, 100, 255};
                                    render_text(small_font, "DNF", GameRenderer::SCREEN_WIDTH - 140, y + 25, red);
                                }
                            }

                            // Mensaje de instrucciones con texto real
                            SDL_Color instr_gray = {150, 150, 150, 255};
                            render_text(small_font, "PRESIONA ESC PARA SALIR", 230, GameRenderer::SCREEN_HEIGHT - 50, instr_gray);

                            renderer.Present();
                            std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        }

                        // Limpiar fuentes
                        if (title_font) TTF_CloseFont(title_font);
                        if (text_font) TTF_CloseFont(text_font);
                        if (small_font) TTF_CloseFont(small_font);

                        std::cout << "[Client] Rankings finales cerrados. Terminando cliente..." << std::endl;
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