#include "client_event_handler.h"

#include <iostream>

ClientEventHandler::ClientEventHandler(Queue<ComandMatchDTO>& cmd_queue, int p_id, bool& running)
    : command_queue(cmd_queue), player_id(p_id), is_running(running),
      valid_keys({SDL_SCANCODE_W,      // Acelerar
                  SDL_SCANCODE_S,      // Frenar
                  SDL_SCANCODE_A,      // Girar izquierda
                  SDL_SCANCODE_D,      // Girar derecha
                  SDL_SCANCODE_SPACE,  // Nitro
                  SDL_SCANCODE_ESCAPE  // Salir
      }) {
    std::cout << "[EventHandler] Inicializado para player " << player_id << std::endl;
}


// PROCESAR MOVIMIENTO (WASD + SPACE)


void ClientEventHandler::process_movement(const SDL_Event& event) {
    if (!valid_keys.contains(event.key.keysym.scancode)) {
        return;
    }

    // Registrar teclas presionadas/soltadas
    if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        pressed_keys.insert(event.key.keysym.scancode);
    } else if (event.type == SDL_KEYUP) {
        pressed_keys.erase(event.key.keysym.scancode);
    }

    // Detectar estado actual de teclas
    bool w = pressed_keys.count(SDL_SCANCODE_W);
    bool s = pressed_keys.count(SDL_SCANCODE_S);
    bool a = pressed_keys.count(SDL_SCANCODE_A);
    bool d = pressed_keys.count(SDL_SCANCODE_D);
    bool space = pressed_keys.count(SDL_SCANCODE_SPACE);

    // Enviar comandos según combinaciones

    // Acelerar (W)
    if (w) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::ACCELERATE;
        cmd.speed_boost = 1.0f;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler]  ACCELERATE enviado" << std::endl;
    }

    // Frenar (S)
    if (s) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::BRAKE;
        cmd.speed_boost = 1.0f;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] BRAKE enviado" << std::endl;
    }

    // Girar izquierda (A)
    if (a) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::TURN_LEFT;
        cmd.turn_intensity = 1.0f;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler]   TURN_LEFT enviado" << std::endl;
    }

    // Girar derecha (D)
    if (d) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::TURN_RIGHT;
        cmd.turn_intensity = 1.0f;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler]  TURN_RIGHT enviado" << std::endl;
    }

    // Nitro (SPACE)
    if (space) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::USE_NITRO;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] ⚡ USE_NITRO enviado" << std::endl;
    }

    // Si no hay ninguna tecla presionada, detener
    if (pressed_keys.empty()) {
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::STOP_ALL;
        command_queue.try_push(cmd);
    }
}


// PROCESAR CHEATS (F1-F4)


void ClientEventHandler::process_cheats(const SDL_Event& event) {
    if (event.type != SDL_KEYDOWN || event.key.repeat != 0) {
        return;
    }

    ComandMatchDTO cmd;
    cmd.player_id = player_id;

    switch (event.key.keysym.scancode) {
    case SDL_SCANCODE_F1:
        // Invencibilidad
        cmd.command = GameCommand::CHEAT_INVINCIBLE;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] CHEAT: Invencibilidad activada" << std::endl;
        break;

    case SDL_SCANCODE_F2:
        // Ganar carrera instantáneamente
        cmd.command = GameCommand::CHEAT_WIN_RACE;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] CHEAT: Ganar carrera" << std::endl;
        break;

    case SDL_SCANCODE_F3:
        // Perder carrera (auto destruido)
        cmd.command = GameCommand::CHEAT_LOSE_RACE;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] CHEAT: Perder carrera" << std::endl;
        break;

    case SDL_SCANCODE_F4:
        // Velocidad máxima instantánea
        cmd.command = GameCommand::CHEAT_MAX_SPEED;
        command_queue.try_push(cmd);
        std::cout << "[EventHandler] CHEAT: Velocidad máxima" << std::endl;
        break;

    default:
        break;
    }
}


// PROCESAR SALIR (ESC o cerrar ventana)

void ClientEventHandler::process_quit(const SDL_Event& event) {
    if (event.type == SDL_QUIT ||
        (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)) {
        std::cout << "[EventHandler] Saliendo del juego..." << std::endl;

        // Enviar comando de desconexión
        ComandMatchDTO cmd;
        cmd.player_id = player_id;
        cmd.command = GameCommand::DISCONNECT;
        command_queue.try_push(cmd);

        is_running = false;
    }
}


// MANEJADOR PRINCIPAL DE EVENTOS


void ClientEventHandler::handle_events() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        // Verificar salida primer
        process_quit(event);

        if (!is_running) {
            return;
        }

        // Procesar cheats (F1-F4)
        process_cheats(event);

        // Procesar movimiento (WASD + SPACE)
        process_movement(event);
    }
}

