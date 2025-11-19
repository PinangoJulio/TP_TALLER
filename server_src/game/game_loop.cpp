#include "game_loop.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <utility>

// ==========================================================
// GAME LOOP IMPLEMENTATION
// ==========================================================

// Definición del constructor para que el linker lo encuentre (vtable)
GameLoop::GameLoop(Queue<ComandMatchDTO> &comandos, ClientMonitor &queues, std::string &yaml_path)
    : is_running(true),
      comandos(comandos),
      queues_players(queues),
      yaml_path(yaml_path)
{
    std::cout << "[GameLoop] Constructor compilado OK. Simulación lista para iniciar." << std::endl;
}

// Implementación del método virtual puro heredado: Thread::run()
void GameLoop::run() {
    is_running = true;
    std::cout << "[GameLoop] Hilo de simulación principal iniciado." << std::endl;

    while (is_running) {
        // --- LÓGICA DEL BUCLE DE JUEGO (Fase 2) ---

        // 1. Procesar Comandos de Entrada
        // Aquí se procesan los comandos de movimiento, cheats, etc., desde la cola.

        // 2. Simular Física (Box2D::Step)

        // 3. Generar y Enviar Snapshot

        // --- Fin de la lógica ---

        // Dormir el hilo para mantener la frecuencia de ticks y liberar CPU
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[GameLoop] Hilo de simulación detenido correctamente." << std::endl;
}

GameLoop::~GameLoop() {
    is_running = false;
}