#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "../../common_src/thread.h"
#include "../../common_src/dtos.h"  

#include "car.h"
#include "player.h"
#include "../network/monitor.h"
#include "server_src/network/client_monitor.h"

#define NITRO_DURATION 12
#define SLEEP 250

/*
 * GameLoop:
 * Recibe Comandos de jugadores (acelerar, frenar, girar, etc)
 * Actualiza la fisica de los autos
 * Detectar colisiones (contra paredes, otros autos, obstaculos YAML)
 * Actualizar estado de juego (vueltas, checkpoints, tiempos)
 * Enviar el estado actualizado a los clientes
 */

class GameLoop : public Thread {
    bool is_running;

    Queue<ComandMatchDTO>& comandos;
    ClientMonitor& queues_players;
    std::vector<Player*> players;
    std::string yaml_path;

    //Mapa mapa;  // construido desde YAML


    void procesar_comandos();
    void actualizar_fisica();
    void detectar_colisiones();
    void actualizar_estado_carrera();
    void enviar_estado_a_jugadores();

public:
    GameLoop(Queue<ComandMatchDTO>& comandos, ClientMonitor& queues, std::string& yaml_path);

    void run() override;
    ~GameLoop() override;
};

#endif //GAME_LOOP_H