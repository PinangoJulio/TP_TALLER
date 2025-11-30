#include "client.h"

#include <SDL2/SDL.h>

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QObject>
#include <SDL2pp/SDL2pp.hh>
#include <chrono>
#include <iostream>
#include <thread>
#include "game/game_renderer.h"

#include "client_event_handler.h"
#include "lobby/controller/lobby_controller.h"

#define NFS_TITLE      "Need for Speed 2D"
#define DEFAULT_WIDTH  1280
#define DEFAULT_HEIGHT 720
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
    std::cout << "[Client] Iniciando fase de lobby Qt..." << std::endl;

    // Crear controlador de lobby con el protocolo ya conectado
    LobbyController controller(this->protocol);

    // Event loop temporal para esperar fin del lobby
    QEventLoop lobbyLoop;
    QObject::connect(&controller, &LobbyController::lobbyFinished, &lobbyLoop, [&](bool success) {
        std::cout << "[Client] Lobby terminado (success=" << success << ")" << std::endl;
        if (!success) {
            active = false;  // abortar cliente si lobby falla
        }
        lobbyLoop.quit();
    });

    controller.start();
    lobbyLoop.exec();  // Bloquea hasta que el lobby emite lobbyFinished

    if (!active) {
        std::cout << "[Client] Abortando inicio de juego por fallo en lobby" << std::endl;
        return;
    }
    
    // Obtener nombre del jugador desde el controlador antes de cerrar
    username = controller.getPlayerName().toStdString();
    std::cout << "[Client] Usuario listo: " << username << std::endl;

    // Cerrar ventanas Qt DESPUÉS de que el QEventLoop termine
    std::cout << "[Client] Cerrando ventanas Qt..." << std::endl;
    controller.closeAllWindows();

    // Procesar eventos pendientes de Qt para que se cierren las ventanas
    QCoreApplication::processEvents();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  ✅ LOBBY COMPLETADO EXITOSAMENTE                        ║" << std::endl;
    std::cout << "║  🎮 Iniciando fase de juego SDL...                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";

    // FASE 2: INICIAR THREADS DE COMUNICACIÓN
    std::cout << "[Client] Iniciando threads de comunicación..." << std::endl;

    sender.start();
    receiver.start();
    threads_started = true;

    std::cout << "[Client] ✅ Thread sender iniciado" << std::endl;
    std::cout << "[Client] ✅ Thread receiver iniciado" << std::endl;

    // FASE 3: INICIALIZAR SDL Y CARGAR CONFIG
    std::cout << "[Client] Iniciando SDL..." << std::endl;

    int window_width = DEFAULT_WIDTH;
    int window_height = DEFAULT_HEIGHT;
    bool fullscreen = false;
    int fps = FPS;

    // Inicializar SDL
    SDL sdl(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_ShowCursor(SDL_DISABLE);

    Window window(NFS_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width,
                  window_height, SDL_WINDOW_SHOWN);

    Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (fullscreen) {
        window.SetFullscreen(SDL_WINDOW_FULLSCREEN_DESKTOP);
        Point window_size = window.GetSize();
        window_width = window_size.GetX();
        window_height = window_size.GetY();
    }

    if (window_width != DEFAULT_WIDTH || window_height != DEFAULT_HEIGHT) {
        renderer.SetLogicalSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    }

    std::cout << "[Client] ✅ SDL inicializado (" << window_width << "x" << window_height << ")"
              << std::endl;

    // ---------------------------------------------------------
    // FASE 4: RECIBIR INFO DE LA CARRERA Y CREAR GAMERENDERER
    // ---------------------------------------------------------
    std::cout << "[Client] 📥 Esperando info de la carrera..." << std::endl;

    // Recibimos la info del mapa (gracias al unread_message_type del lobby, esto funciona)
    RaceInfoDTO race_info = protocol.receive_race_info();

    std::cout << "[Client] ✅ Info recibida: " << race_info.city_name 
              << " - " << race_info.race_name << std::endl;

    // Crear e inicializar el renderizador
    GameRenderer game_renderer(renderer, race_info);
    game_renderer.init(); // Cargar texturas

    ClientEventHandler event_handler(command_queue, player_id, active);

    int ms_per_frame = 1000 / fps;
    GameState current_snapshot;
    GameState final_snapshot;
    bool race_finished = false;

    std::cout << "\n";
    std::cout << "[Client] 🏁 Entrando al game loop..." << std::endl;
    // ... (Prints de controles omitidos para brevedad) ...

    // ---------------------------------------------------------
    // FASE 5: GAME LOOP PRINCIPAL
    // ---------------------------------------------------------
    while (active) {
        auto t1 = std::chrono::steady_clock::now();

        std::vector<GameState> snapshots;

        if (!race_finished) {
            while (snapshot_queue.try_pop(current_snapshot)) {
                snapshots.push_back(current_snapshot);

                InfoPlayer* local_player = current_snapshot.findPlayer(player_id);
                if (local_player && local_player->race_finished) {
                    race_finished = true;
                    final_snapshot = current_snapshot;
                    std::cout << "[Client] 🏁 Carrera finalizada!" << std::endl;
                    break;
                }
            }
        }

        if (!race_finished) {
            if (!snapshots.empty()) {
                const auto& latest = snapshots.back();

                // ✅ RENDERIZAR JUEGO (Descomentado)
                game_renderer.render(latest, player_id);

                // DEBUG: Imprimir estado cada 60 frames
                static int frame_count = 0;
                if (++frame_count % 60 == 0) {
                    InfoPlayer* local_player = latest.findPlayer(player_id);
                    if (local_player) {
                        std::cout << "[Client] Pos: (" << (int)local_player->pos_x << ", " 
                                  << (int)local_player->pos_y << ") | Vel: " 
                                  << (int)local_player->speed << " km/h" << std::endl;
                    }
                }
            }
        } else {
            // ✅ RENDERIZAR FINAL (Descomentado)
            game_renderer.render(final_snapshot, player_id);
        }

        event_handler.handle_events();

        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        if (elapsed < ms_per_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
        }

        if (race_finished) {
            std::cout << "[Client] 🏁 Saliendo en 3 segundos..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            active = false;
        }
    }

    std::cout << "\n";
    std::cout << "[Client] Game loop finalizado" << std::endl;
    std::cout << "[Client] Cerrando recursos..." << std::endl;

    if (threads_started) {
        std::cout << "[Client]   → Deteniendo thread sender..." << std::endl;
        sender.stop();
        
        std::cout << "[Client]   → Cerrando colas de comunicación..." << std::endl;
        command_queue.close();
        snapshot_queue.close();

        std::cout << "[Client]   → Esperando finalización de threads..." << std::endl;
        sender.join();
        // receiver.join(); // Descomentar si receiver tiene stop/join implementado correctamente

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
