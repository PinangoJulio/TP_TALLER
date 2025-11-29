#include <chrono>
#include <iostream>
#include <thread>

#include "../../common_src/dtos.h"
#include "../../common_src/queue.h"
#include "../../common_src/game_state.h"
#include "../game/game_loop.h"
#include "../network/client_monitor.h"

// Pequeña utilidad para esperar ms
static void sleep_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

int main() {
    std::cout << "==== TEST MOVIMIENTO AUTO & CHEATS ====" << std::endl;

    // Ruta de mapa simplificada (usar config.yaml para evitar dependencia de archivos específicos)
    const std::string mapa_yaml = "config.yaml"; // Se usará sólo para intentar leer spawn_points

    // Queues
    Queue<ComandMatchDTO> comandos;
    Queue<GameState> snapshots;

    // Monitor y registro de queue del cliente (player_id = 1)
    ClientMonitor monitor;
    monitor.add_client_queue(snapshots, 1);

    // Crear GameLoop
    GameLoop loop(comandos, monitor, mapa_yaml, "vice city");

    // Agregar jugador de prueba
    loop.add_player(1, "tester", "Brisa", "sport");

    // Iniciar hilo del loop
    loop.start();

    // Enviar algunos comandos de movimiento
    for (int i = 0; i < 5; ++i) {
        ComandMatchDTO accel; accel.player_id = 1; accel.command = GameCommand::ACCELERATE; accel.speed_boost = 1.0f;
        comandos.push(accel);
        sleep_ms(300);
    }

    // Activar nitro
    ComandMatchDTO nitro; nitro.player_id = 1; nitro.command = GameCommand::USE_NITRO;
    comandos.push(nitro);
    sleep_ms(500);

    // Cheat: velocidad máxima
    ComandMatchDTO maxspd; maxspd.player_id = 1; maxspd.command = GameCommand::CHEAT_MAX_SPEED;
    comandos.push(maxspd);
    sleep_ms(500);

    // Cheat: invencible
    ComandMatchDTO inv; inv.player_id = 1; inv.command = GameCommand::CHEAT_INVINCIBLE;
    comandos.push(inv);
    sleep_ms(300);

    // Cheat: ganar carrera (termina el loop)
    ComandMatchDTO win; win.player_id = 1; win.command = GameCommand::CHEAT_WIN_RACE;
    comandos.push(win);

    // Consumir snapshots hasta que el loop termine
    while (loop.is_alive()) {
        GameState snap;
        while (snapshots.try_pop(snap)) {
            auto* info = snap.findPlayer(1);
            if (info) {
                std::cout << "[Snapshot] pos=(" << info->pos_x << "," << info->pos_y << ") speed=" << info->speed
                          << " health=" << info->health << " nitro=" << info->nitro_amount
                          << " finished=" << info->race_finished << std::endl;
            }
        }
        sleep_ms(200);
    }

    loop.join();
    std::cout << "==== FIN TEST MOVIMIENTO AUTO ====" << std::endl;
    return 0;
}
