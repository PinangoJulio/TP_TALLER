#ifndef CLIENT_EVENT_HANDLER_H
#define CLIENT_EVENT_HANDLER_H

#include <SDL2/SDL.h>
#include <unordered_set>

#include "../common_src/dtos.h"
#include "../common_src/queue.h"

class ClientEventHandler {
private:
    Queue<ComandMatchDTO>& command_queue;
    int player_id;
    bool& is_running;

    // Teclas válidas para controles
    std::unordered_set<SDL_Scancode> valid_keys;
    std::unordered_set<SDL_Scancode> pressed_keys;

    // Procesadores de eventos específicos
    void process_movement(const SDL_Event& event);
    void process_cheats(const SDL_Event& event);
    void process_quit(const SDL_Event& event);

public:
    ClientEventHandler(Queue<ComandMatchDTO>& cmd_queue, int p_id, bool& running);

    // Método principal que maneja todos los eventos SDL
    void handle_events();

    ~ClientEventHandler() = default;
};

#endif  // CLIENT_EVENT_HANDLER_H
