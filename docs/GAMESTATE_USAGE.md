# Guía de Uso: GameState (Snapshot del Juego)

## Introducción

El `GameState` es la estructura que representa un **snapshot completo del estado del juego** en un momento dado. Se crea en el **GameLoop**, se serializa en el **ServerProtocol**, y se deserializa en el **ClientProtocol** para que los clientes puedan renderizar el juego.

---

## 1. Crear Snapshot en GameLoop

```cpp
// Dentro de GameLoop::run() o GameLoop::create_snapshot()
GameState GameLoop::create_snapshot() {
    // 1️⃣ Crear snapshot base con todos los jugadores
    GameState snapshot(players);  // Constructor llena info de todos los players

    // 2️⃣ Llenar información del circuito actual
    snapshot.race_current_info.city = current_city;              // "Vice City"
    snapshot.race_current_info.race_name = current_map;          // "Playa"
    snapshot.race_current_info.total_laps = config_total_laps;   // 3
    snapshot.race_current_info.total_checkpoints = checkpoints.size();

    // 3️⃣ Llenar información de la carrera
    snapshot.race_info.status = current_status;           // IN_PROGRESS
    snapshot.race_info.race_number = current_race_num;    // 1
    snapshot.race_info.total_races = total_races;         // 3
    snapshot.race_info.remaining_time_ms = calculate_remaining_time();
    snapshot.race_info.players_finished = count_finished_players();
    snapshot.race_info.total_players = players.size();
    snapshot.race_info.winner_name = get_current_winner();

    // 4️⃣ Llenar checkpoints
    for (const auto& cp : checkpoints) {
        CheckpointInfo cp_info;
        cp_info.id = cp.id;
        cp_info.pos_x = cp.x;
        cp_info.pos_y = cp.y;
        cp_info.width = cp.width;
        cp_info.angle = cp.angle;
        cp_info.is_start = (cp.id == 0);
        cp_info.is_finish = (cp.id == checkpoints.size() - 1);
        snapshot.checkpoints.push_back(cp_info);
    }

    // 5️⃣ Llenar hints (flechas direccionales)
    for (const auto& hint : hints) {
        HintInfo hint_info;
        hint_info.id = hint.id;
        hint_info.pos_x = hint.x;
        hint_info.pos_y = hint.y;
        hint_info.direction_angle = hint.direction;
        hint_info.for_checkpoint = hint.target_checkpoint;
        snapshot.hints.push_back(hint_info);
    }

    // 6️⃣ Llenar NPCs
    for (const auto& npc : npc_cars) {
        NPCCarInfo npc_info;
        npc_info.npc_id = npc.id;
        npc_info.pos_x = npc.x;
        npc_info.pos_y = npc.y;
        npc_info.angle = npc.angle;
        npc_info.speed = npc.speed;
        npc_info.car_model = npc.model;
        npc_info.is_parked = npc.parked;
        snapshot.npcs.push_back(npc_info);
    }

    // 7️⃣ Llenar eventos recientes (explosiones, colisiones)
    snapshot.events = recent_events;
    recent_events.clear();  // Limpiar para próximo frame

    return snapshot;
}
```

---

## 2. Serializar en ServerProtocol

```cpp
bool ServerProtocol::send_snapshot(const GameState& snapshot) {
    std::vector<uint8_t> buffer;

    // Prefijo: tipo de mensaje
    buffer.push_back(static_cast<uint8_t>(ServerMessageType::GAME_SNAPSHOT));

    // A) JUGADORES
    push_uint16(buffer, static_cast<uint16_t>(snapshot.players.size()));

    for (const auto& player : snapshot.players) {
        push_uint32(buffer, static_cast<uint32_t>(player.player_id));
        push_string(buffer, player.username);
        push_string(buffer, player.car_name);
        push_string(buffer, player.car_type);

        // Posición (multiplicamos por 100 para enviar como enteros)
        push_uint32(buffer, static_cast<uint32_t>(player.pos_x * 100.0f));
        push_uint32(buffer, static_cast<uint32_t>(player.pos_y * 100.0f));
        push_uint16(buffer, static_cast<uint16_t>(player.angle * 100.0f));
        push_uint16(buffer, static_cast<uint16_t>(player.speed * 100.0f));

        // Estado
        buffer.push_back(static_cast<uint8_t>(player.health));
        buffer.push_back(player.nitro_active ? 0x01 : 0x00);
        // ... más campos
    }

    // B) CHECKPOINTS, C) HINTS, D) NPCs, E) RACE INFO...
    // (Ver código completo en game_state.h)

    skt.sendall(buffer.data(), buffer.size());
    return true;
}
```

---

## 3. Deserializar en ClientProtocol

```cpp
GameState ClientProtocol::receive_snapshot() {
    GameState snapshot;

    // A) JUGADORES
    uint16_t num_players = read_uint16();
    for (uint16_t i = 0; i < num_players; ++i) {
        InfoPlayer player;
        player.player_id = read_uint32();
        player.username = read_string();
        player.car_name = read_string();
        player.car_type = read_string();

        // Recibimos multiplicado por 100, dividimos
        player.pos_x = read_uint32() / 100.0f;
        player.pos_y = read_uint32() / 100.0f;
        player.angle = read_uint16() / 100.0f;
        player.speed = read_uint16() / 100.0f;

        player.health = read_uint8();
        player.nitro_active = read_uint8() == 0x01;

        snapshot.players.push_back(player);
    }

    // B) CHECKPOINTS, C) HINTS, etc...

    return snapshot;
}
```

---

## 4. Broadcast en Sender

```cpp
void Sender::run() {
    while (running) {
        // Esperar snapshot del GameLoop
        GameState snapshot = snapshot_queue.pop();

        // Broadcast a todos los jugadores
        for (auto& [player_id, socket] : player_sockets) {
            try {
                ServerProtocol protocol(*socket);
                protocol.send_snapshot(snapshot);
            } catch (...) {
                // Manejar error
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(33));  // ~30 FPS
    }
}
```

---

## Frecuencia de Envío

- **30 FPS**: Cada 33ms (bueno para juegos rápidos)
- **60 FPS**: Cada 16ms (más fluido, más ancho de banda)
- **20 FPS**: Cada 50ms (menor uso de red)

---

## Notas Importantes

### Multiplicar por 100
Para evitar usar floats en el protocolo, multiplicamos por 100 y enviamos como enteros:
```cpp
// Enviar
push_uint32(buffer, static_cast<uint32_t>(pos_x * 100.0f));

// Recibir
pos_x = read_uint32() / 100.0f;
```

### Campos Opcionales
Algunos campos pueden no ser necesarios en todos los snapshots. Considera:
- Enviar **solo jugadores visibles** para el cliente
- Enviar **solo checkpoints cercanos**
- Enviar **solo NPCs en el viewport**

### Compresión
Para partidas con muchos jugadores, considera comprimir el snapshot con zlib o similar.

---

## Archivo de Referencia

Ver la implementación completa en:
- `common_src/game_state.h` - Definición de estructuras
- `common_src/game_state.cpp` - Constructor y helpers
- `server_src/server_protocol.cpp` - Serialización (TODO)
- `client_src/client_protocol.cpp` - Deserialización (TODO)

