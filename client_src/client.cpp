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

// #include "game/game_renderer.h"  // TODO: Implementar GameRenderer
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
    // Usamos una referencia explícita al protocolo de la clase
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
    username = controller.getPlayerName().toStdString();
    std::cout << "[Client] Usuario listo: " << username << std::endl;
    // Cerrar ventanas Qt DESPUÉS de que el QEventLoop termine
    std::cout << "[Client] Cerrando ventanas Qt..." << std::endl;
    controller.closeAllWindows();

    // Procesar eventos pendientes de Qt para que se cierren las ventanas
    QCoreApplication::processEvents();

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  LOBBY COMPLETADO EXITOSAMENTE                        ║" << std::endl;
    std::cout << "║   Iniciando fase de juego SDL...                       ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";

    // FASE 2: INICIAR THREADS DE COMUNICACIÓN

    std::cout << "[Client] Iniciando threads de comunicación..." << std::endl;

    sender.start();
    // TODO: Descomentar cuando el servidor envíe snapshots correctamente
    // receiver.start();
    threads_started = true;

    std::cout << "[Client] ✅ Thread sender iniciado (receiver desactivado temporalmente)"
              << std::endl;

    // FASE 3: INICIALIZAR SDL Y CARGAR CONFIG

    std::cout << "[Client] Iniciando SDL..." << std::endl;

    // Cargar configuración desde config.yaml
    // Config::load("config.yaml");
    // int window_width = Config::get<int>("window_width");
    // int window_height = Config::get<int>("window_height");
    // bool fullscreen = Config::get<bool>("fullscreen");
    // int fps = Config::get<int>("fps");

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

    // Configurar fullscreen si está habilitado
    if (fullscreen) {
        window.SetFullscreen(SDL_WINDOW_FULLSCREEN_DESKTOP);
        Point window_size = window.GetSize();
        window_width = window_size.GetX();
        window_height = window_size.GetY();
    }

    // Configurar escala lógica si es necesario
    if (window_width != DEFAULT_WIDTH || window_height != DEFAULT_HEIGHT) {
        renderer.SetLogicalSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
    }

    std::cout << "[Client] ✅ SDL inicializado (" << window_width << "x" << window_height << ")"
              << std::endl;

    // FASE 4: CREAR COMPONENTES DEL JUEGO

    //  Recibir mapa inicial del servidor  --> para renderizar
    // std::string initial_map = protocol.receive_initial_map();

    // Implementar GameRenderer
    // GameRenderer game_renderer(renderer, player_id, window_width, window_height);

    //  Implementar EventHandler para teclado
    // EventHandler event_handler(command_queue, player_id);

    // Implementar sistema de sonido
    // SoundManager sound_manager(player_id);

    int ms_per_frame = 1000 / fps;
    GameState current_snapshot;
    GameState final_snapshot;
    bool race_finished = false;

    std::cout << "\n";
    std::cout << "[Client]Entrando al game loop..." << std::endl;
    std::cout << "[Client]  Controles:" << std::endl;
    std::cout << "[Client]    ↑/W: Acelerar" << std::endl;
    std::cout << "[Client]    ↓/S: Frenar" << std::endl;
    std::cout << "[Client]    ←/A: Girar izquierda" << std::endl;
    std::cout << "[Client]    →/D: Girar derecha" << std::endl;
    std::cout << "[Client]    ESPACIO: Usar nitro" << std::endl;
    std::cout << "[Client]    ESC: Salir del juego" << std::endl;
    std::cout << "\n";

    // FASE 5: GAME LOOP PRINCIPAL

    while (active) {
        auto t1 = std::chrono::steady_clock::now();

        // 1. Leer todos los snapshots disponibles de la queue
        std::vector<GameState> snapshots;

        if (!race_finished) {
            while (snapshot_queue.try_pop(current_snapshot)) {
                snapshots.push_back(current_snapshot);

                // Verificar si la carrera terminó (todos los jugadores terminaron)
                // Por ahora, verificamos si el jugador local terminó
                InfoPlayer* local_player = current_snapshot.findPlayer(player_id);
                if (local_player && local_player->race_finished) {
                    race_finished = true;
                    final_snapshot = current_snapshot;  // Guardar snapshot final
                    std::cout << "[Client]  Carrera finalizada!" << std::endl;
                    break;
                }
            }
        }

        // 2. Reproducir sonidos y renderizar
        if (!race_finished) {
            // Reproducir sonidos de todos los snapshots recibidos
            //  Implementar SoundManager
            // for (const auto& snapshot : snapshots) {
            //     sound_manager.play_sounds(snapshot);
            // }

            // Renderizar el último snapshot
            if (!snapshots.empty()) {
                const auto& latest = snapshots.back();

                // TODO: game_renderer.render(latest);

                // DEBUG: Imprimir estado básico
                InfoPlayer* local_player = latest.findPlayer(player_id);
                std::cout << "[Client] Players: " << latest.players.size();
                if (local_player) {
                    std::cout << " | Lap: " << local_player->completed_laps << "/"
                              << latest.race_current_info.total_laps;
                }
                std::cout << std::endl;
            }
        } else {
            // Si la carrera terminó, seguir mostrando el snapshot final
            // game_renderer.render(final_snapshot);
        }

        // 3. Manejar eventos del jugador (teclado)
        //  event_handler.handle_events(active);

        // Manejo básico de eventos SDL
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                active = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    active = false;
                    break;
                case SDLK_UP:
                case SDLK_w: {
                    // Enviar comando ACCELERATE
                    ComandMatchDTO cmd;
                    cmd.player_id = player_id;
                    cmd.command = GameCommand::ACCELERATE;
                    command_queue.push(cmd);
                    break;
                }
                case SDLK_DOWN:
                case SDLK_s: {
                    // Enviar comando BRAKE
                    ComandMatchDTO cmd;
                    cmd.player_id = player_id;
                    cmd.command = GameCommand::BRAKE;
                    command_queue.push(cmd);
                    break;
                }
                case SDLK_LEFT:
                case SDLK_a: {
                    // Enviar comando TURN_LEFT
                    ComandMatchDTO cmd;
                    cmd.player_id = player_id;
                    cmd.command = GameCommand::TURN_LEFT;
                    command_queue.push(cmd);
                    break;
                }
                case SDLK_RIGHT:
                case SDLK_d: {
                    // Enviar comando TURN_RIGHT
                    ComandMatchDTO cmd;
                    cmd.player_id = player_id;
                    cmd.command = GameCommand::TURN_RIGHT;
                    command_queue.push(cmd);
                    break;
                }
                case SDLK_SPACE: {
                    // Enviar comando USE_NITRO
                    ComandMatchDTO cmd;
                    cmd.player_id = player_id;
                    cmd.command = GameCommand::USE_NITRO;
                    command_queue.push(cmd);
                    break;
                }
                }
            }
        }

        // 4. Control de FPS
        auto t2 = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        if (elapsed < ms_per_frame) {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms_per_frame - elapsed));
        }

        // Si la carrera terminó, esperar un poco antes de salir
        if (race_finished) {
            std::cout << "[Client]  Carrera finalizada. Saliendo en 3 segundos..." << std::endl;
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
        //  Descomentar cuando receiver esté activo
        // receiver.stop();

        std::cout << "[Client]   → Cerrando colas de comunicación..." << std::endl;
        command_queue.close();
        snapshot_queue.close();

        std::cout << "[Client]   → Esperando finalización de threads..." << std::endl;
        sender.join();
        //  Descomentar cuando receiver esté activo
        // receiver.join();

        std::cout << "[Client]   ✅ Threads finalizados correctamente" << std::endl;
        threads_started = false;  // Marcar como cerrados para evitar doble cierre en destructor
    }

    std::cout << "[Client] ✅ Todos los recursos cerrados correctamente" << std::endl;
}

Client::~Client() {
    std::cout << "[Client] Destructor llamado" << std::endl;

    // Solo cerrar threads si no fueron cerrados en start()
    if (threads_started) {
        std::cout << "[Client] Threads aún activos, cerrando desde destructor..." << std::endl;

        sender.stop();
        // TODO: Descomentar cuando receiver esté activo
        // receiver.stop();

        command_queue.close();
        snapshot_queue.close();

        sender.join();
        // TODO: Descomentar cuando receiver esté activo
        // receiver.join();

        std::cout << "[Client] ✅ Threads finalizados desde destructor" << std::endl;
    } else {
        std::cout << "[Client] Threads ya fueron cerrados previamente" << std::endl;
    }

    std::cout << "[Client] Destructor completado" << std::endl;
}
