# Arquitectura del Cliente - Need for Speed 2D

## Estructura General

El cliente sigue un flujo de **3 fases secuenciales**:

```
FASE 1: LOBBY (Qt)  →  FASE 2: THREADS  →  FASE 3: GAME LOOP (SDL)
```

---

## Flujo Detallado

### FASE 1: LOBBY (Qt)

**Responsable**: `LobbyController`

1. Usuario ingresa su nombre
2. Elige crear o unirse a una partida
3. Selecciona su auto
4. Espera a que otros jugadores se unan
5. Cuando el host inicia, el lobby finaliza exitosamente

**Resultado**: El cliente obtiene:
- `player_id`: ID del jugador asignado por el servidor
- `selected_car`: Auto elegido
- `match_id`: ID de la partida
- Configuración de la carrera

---

### FASE 2: THREADS DE COMUNICACIÓN

Se inician dos threads en paralelo:

#### **ClientSender**
- Lee de `command_queue`
- Serializa comandos (`ComandMatchDTO`)
- Envía al servidor via `ClientProtocol`

#### **ClientReceiver**
- Recibe snapshots del servidor
- Deserializa `GameStateSnapshot`
- Pushea a `snapshot_queue`

**Comunicación**:
```
[Teclado] → command_queue → ClientSender → [Servidor]
[Servidor] → ClientReceiver → snapshot_queue → [Renderizado]
```

---

### FASE 3: GAME LOOP (SDL)

**Componentes principales** (a implementar):

1. **GameRenderer**: Renderiza el estado del juego
2. **EventHandler**: Captura teclas y genera comandos
3. **SoundManager**: Reproduce sonidos según eventos

**Loop principal** (60 FPS):

```cpp
while (active) {
    // 1. Leer todos los snapshots disponibles
    while (snapshot_queue.try_pop(snapshot)) {
        snapshots.push(snapshot);
        if (snapshot.race_finished) {
            race_finished = true;
        }
    }
    
    // 2. Reproducir sonidos
    for (snapshot : snapshots) {
        sound_manager.play_sounds(snapshot);
    }
    
    // 3. Renderizar último snapshot
    if (!snapshots.empty()) {
        game_renderer.render(snapshots.back());
    }
    
    // 4. Manejar eventos de teclado
    event_handler.handle_events(active);
    
    // 5. Control de FPS
    sleep_until_next_frame();
}
```

---

## Estructura de Clases

### `Client` (principal)
```cpp
class Client {
    ClientProtocol protocol;           // Comunicación con servidor
    std::string username;              // Nombre del jugador
    uint16_t player_id;                // ID asignado por servidor
    bool active;                       // Estado del game loop
    
    Queue<ComandMatchDTO> command_queue;       // Comandos → servidor
    Queue<GameStateSnapshot> snapshot_queue;   // Servidor → renderizado
    
    ClientSender sender;               // Thread que envía comandos
    ClientReceiver receiver;           // Thread que recibe snapshots
    
    void start();                      // Flujo completo: lobby → threads → game loop
};
```

### `ClientProtocol`
```cpp
class ClientProtocol {
    Socket socket;
    
    // Lobby
    void send_username(string);
    void create_game(...);
    void join_game(uint16_t);
    void select_car(string, string);
    void start_game(uint16_t);
    
    // Game
    void send_command_client(ComandMatchDTO);
    GameState receive_snapshot();
};
```

### `ClientSender` (Thread)
```cpp
class ClientSender : public Thread {
    ClientProtocol& protocol;
    Queue<ComandMatchDTO>& queue;
    
    void run() override {
        while (running) {
            ComandMatchDTO cmd = queue.pop();
            protocol.send_command_client(cmd);
        }
    }
};
```

### `ClientReceiver` (Thread)
```cpp
class ClientReceiver : public Thread {
    ClientProtocol& protocol;
    Queue<GameStateSnapshot>& queue;
    
    void run() override {
        while (running) {
            GameStateSnapshot snapshot = protocol.receive_snapshot();
            queue.push(snapshot);
        }
    }
};
```

---

## Comandos del Jugador

### `ComandMatchDTO`
```cpp
struct ComandMatchDTO {
    int player_id;
    GameCommand command;  // ACCELERATE, BRAKE, TURN_LEFT, TURN_RIGHT, USE_NITRO
};
```

### Mapeo de teclas:
- **W / ↑**: ACCELERATE
- **S / ↓**: BRAKE
- **A / ←**: TURN_LEFT
- **D / →**: TURN_RIGHT
- **SPACE**: USE_NITRO
- **ESC**: Salir

---

## Estado del Juego (Snapshot)

### `GameStateSnapshot`
```cpp
struct GameStateSnapshot {
    std::vector<InfoPlayer> players;   // Estado de todos los jugadores
    std::string city_name;             // Ciudad actual
    std::string map_path;              // Ruta del mapa
    int current_lap;                   // Vuelta actual
    int total_laps;                    // Total de vueltas
    bool race_finished;                // ¿Carrera terminada?
};
```

### `InfoPlayer`
```cpp
struct InfoPlayer {
    int player_id;
    std::string name;
    float pos_x, pos_y;
    float angle;
    float velocity;
    int current_checkpoint;
    int laps_completed;
    bool finished;
    
    InfoCar car;                       // Estado del auto
};
```

---

## TODOs Pendientes

### Alta prioridad:
1. ✅ Estructura de `Client` con queues y threads
2. ✅ Flujo completo: lobby → threads → SDL
3. ⏳ **Implementar `GameRenderer`**: Renderizar mapa, autos, UI
4. ⏳ **Implementar `EventHandler`**: Capturar teclas y generar comandos
5. ⏳ **Deserialización de snapshot** en `ClientProtocol::receive_snapshot()`

### Media prioridad:
6. ⏳ **Implementar `SoundManager`**: Sonidos de motor, choques, nitro
7. ⏳ **Cargar configuración desde `config.yaml`**:
   - window_width, window_height
   - fullscreen
   - fps
   - volume

### Baja prioridad:
8. ⏳ **Integración completa con LobbyController**:
   - Obtener `player_id`, `selected_car`, `match_id`
   - Verificar si el lobby salió bien antes de continuar
9. ⏳ **Pantalla de resultados** al finalizar carrera
10. ⏳ **Minimapa**

---

## Diagrama de Flujo

```
┌──────────────┐
│    main()    │
└──────┬───────┘
       │
       ▼
┌──────────────────────────────────────────┐
│ Client client(host, port, username)      │
│ client.start()                           │
└──────┬───────────────────────────────────┘
       │
       ▼
┌─────────────────────────────────────────────┐
│ FASE 1: LOBBY (Qt)                          │
│  - LobbyController.start()                  │
│  - Usuario ingresa nombre                   │
│  - Crea/une a partida                       │
│  - Selecciona auto                          │
│  - Espera otros jugadores                   │
│  - Host inicia → lobby finaliza             │
└─────────┬───────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────┐
│ FASE 2: THREADS                             │
│  - sender.start()                           │
│  - receiver.start()                         │
└─────────┬───────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────┐
│ FASE 3: SDL GAME LOOP                       │
│  - Inicializar SDL (ventana, renderer)     │
│  - Crear GameRenderer, EventHandler         │
│  - Loop principal (60 FPS):                 │
│    ┌─────────────────────────────────┐     │
│    │ while (active) {                │     │
│    │   1. Leer snapshots              │     │
│    │   2. Reproducir sonidos          │     │
│    │   3. Renderizar                  │     │
│    │   4. Manejar eventos teclado     │     │
│    │   5. Control FPS                 │     │
│    │ }                                │     │
│    └─────────────────────────────────┘     │
└─────────┬───────────────────────────────────┘
          │
          ▼
┌─────────────────────────────────────────────┐
│ Destructor: detener threads y cerrar queues│
└─────────────────────────────────────────────┘
```

---

## Comunicación Cliente-Servidor

### Durante el juego:

**Cliente → Servidor**:
```
[Jugador presiona W] 
    → EventHandler captura tecla
    → Crea ComandMatchDTO{player_id, ACCELERATE}
    → Push a command_queue
    → ClientSender.run() hace pop
    → Serializa y envía por socket
    → [Servidor]
```

**Servidor → Cliente**:
```
[Servidor ejecuta GameLoop]
    → Genera GameStateSnapshot
    → Serializa y envía por socket
    → ClientReceiver.run() recibe
    → Deserializa a GameStateSnapshot
    → Push a snapshot_queue
    → Game loop hace pop y renderiza
```

---

## Notas Importantes

1. **Queues bloqueantes**: `command_queue` y `snapshot_queue` son bloqueantes para sincronizar threads

2. **Ownership de threads**: El destructor de `Client` se asegura de detener los threads limpiamente

3. **Separación de responsabilidades**:
   - `Client`: Orquesta el flujo general
   - `LobbyController`: Maneja toda la UI de Qt
   - `GameRenderer`: Solo renderiza, no tiene lógica
   - `EventHandler`: Solo captura eventos, no tiene lógica
   - Threads: Solo I/O de red, no procesan datos

4. **FPS independiente de red**: El game loop corre a 60 FPS fijos, independiente de cuántos snapshots lleguen del servidor

