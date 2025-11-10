# 🚧 Tareas Pendientes para Integración Completa del Lobby

## 1. Protocolo - Mensajes Faltantes

### En `lobby_protocol.h`:
Agregar estos tipos de mensaje al enum `LobbyMessageType`:
```cpp
enum LobbyMessageType : uint8_t {
    // ... mensajes existentes ...
    
    // ✅ YA EXISTE: MSG_START_GAME = 0x05
    MSG_SELECT_CAR = 0x06,       // Cliente → Servidor: Seleccionar auto
    MSG_LEAVE_GAME = 0x07,       // Cliente → Servidor: Abandonar partida
    MSG_PLAYER_READY = 0x08,     // Cliente → Servidor: Marcar como listo
    
    // Notificaciones del servidor
    MSG_PLAYER_JOINED_NOTIFICATION = 0x15,   // Servidor → Clientes: Jugador se unió
    MSG_PLAYER_LEFT_NOTIFICATION = 0x16,     // Servidor → Clientes: Jugador salió
    MSG_PLAYER_READY_NOTIFICATION = 0x17,    // Servidor → Clientes: Estado ready cambió
    MSG_CAR_SELECTED_NOTIFICATION = 0x18,    // Servidor → Clientes: Auto seleccionado
};
```

### En `lobby_protocol.cpp`:
Implementar funciones de serialización:
```cpp
namespace LobbyProtocol {
    // Cliente → Servidor
    std::vector<uint8_t> serialize_select_car(uint8_t car_index);
    std::vector<uint8_t> serialize_leave_game(uint16_t game_id);
    std::vector<uint8_t> serialize_player_ready(bool is_ready);
    std::vector<uint8_t> serialize_start_game(uint16_t game_id); // Ya existe pero verificar
    
    // Servidor → Clientes (notificaciones broadcast)
    std::vector<uint8_t> serialize_player_joined_notification(
        uint16_t player_id, 
        const std::string& player_name
    );
    
    std::vector<uint8_t> serialize_player_left_notification(uint16_t player_id);
    
    std::vector<uint8_t> serialize_player_ready_notification(
        uint16_t player_id, 
        bool is_ready
    );
    
    std::vector<uint8_t> serialize_car_selected_notification(
        uint16_t player_id, 
        uint8_t car_index
    );
}
```

---

## 2. Servidor - Lógica de Sala de Espera

### Estados del jugador en la partida:
```cpp
enum class PlayerState {
    WAITING,      // En sala de espera, sin auto seleccionado
    READY,        // Listo para empezar
    IN_GAME       // Jugando
};

struct PlayerInGame {
    uint16_t player_id;
    std::string username;
    uint8_t car_index;
    PlayerState state;
    bool is_host;  // Solo el host puede iniciar
};
```

### Funcionalidades requeridas:

#### A. **Manejo de MSG_SELECT_CAR**
```
1. Recibir del cliente: car_index (uint8_t)
2. Validar que el jugador esté en la partida
3. Guardar selección del auto en PlayerInGame
4. Broadcast a todos los jugadores: MSG_CAR_SELECTED_NOTIFICATION
```

#### B. **Manejo de MSG_PLAYER_READY**
```
1. Recibir del cliente: is_ready (bool)
2. Validar que el jugador tenga auto seleccionado
3. Actualizar estado del jugador
4. Broadcast a todos: MSG_PLAYER_READY_NOTIFICATION
5. Si todos están listos → notificar al host que puede iniciar
```

#### C. **Manejo de MSG_START_GAME**
```
1. Recibir del cliente: game_id (uint16_t)
2. Validar que el cliente sea el HOST
3. Validar que todos los jugadores estén READY
4. Validar mínimo 2 jugadores
5. Marcar partida como iniciada (is_started = true)
6. Broadcast a todos: MSG_GAME_STARTED
7. Transicionar al bucle de juego (SDL/Physics loop)
```

#### D. **Manejo de MSG_LEAVE_GAME**
```
1. Recibir del cliente: game_id (uint16_t)
2. Remover jugador de la partida
3. Si era el host → asignar nuevo host
4. Broadcast a todos: MSG_PLAYER_LEFT_NOTIFICATION
5. Si quedan 0 jugadores → eliminar la partida
```

---

## 3. Cliente - Receptor de Notificaciones (Opcional pero recomendado)

### En `lobby_client.cpp`:
Agregar un thread/loop que escuche mensajes del servidor mientras está en waiting room:
```cpp
// Método para recibir notificaciones asíncronas
void LobbyClient::process_notifications() {
    while (in_waiting_room) {
        uint8_t msg_type = peek_message_type();
        
        switch (msg_type) {
            case MSG_PLAYER_JOINED_NOTIFICATION:
                handle_player_joined();
                break;
            case MSG_PLAYER_LEFT_NOTIFICATION:
                handle_player_left();
                break;
            case MSG_PLAYER_READY_NOTIFICATION:
                handle_player_ready_changed();
                break;
            case MSG_CAR_SELECTED_NOTIFICATION:
                handle_car_selected();
                break;
            case MSG_GAME_STARTED:
                handle_game_started();
                break;
            default:
                break;
        }
    }
}
```

---

## 4. Integración Final

### Una vez implementado en el servidor:

#### En `lobby_client.cpp`, reemplazar placeholders:
```cpp
void LobbyClient::select_car(uint8_t car_index) {
    auto buffer = LobbyProtocol::serialize_select_car(car_index);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Selected car: " << static_cast<int>(car_index) << std::endl;
}

void LobbyClient::set_ready(bool is_ready) {
    auto buffer = LobbyProtocol::serialize_player_ready(is_ready);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Ready state: " << (is_ready ? "YES" : "NO") << std::endl;
}

void LobbyClient::leave_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_leave_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Left game: " << game_id << std::endl;
}

void LobbyClient::start_game(uint16_t game_id) {
    auto buffer = LobbyProtocol::serialize_start_game(game_id);
    socket.sendall(buffer.data(), buffer.size());
    std::cout << "[LobbyClient] Start game request: " << game_id << std::endl;
}
```

---

## 5. Testing Recomendado

### Casos de prueba esenciales:

1. ✅ **Flujo completo**: Login → Crear partida → Seleccionar auto → Ready → Iniciar
2. ✅ **Dos clientes**: Uno crea, otro se une, ambos listos, host inicia
3. ✅ **Abandonar partida**: Desde garage, desde waiting room
4. ✅ **Host abandona**: Verificar reasignación de host
5. ✅ **Partida llena**: Verificar rechazo al intentar unirse
6. ✅ **Iniciar sin estar ready**: Verificar error
7. ✅ **No-host intenta iniciar**: Verificar error ERR_NOT_HOST

---

## 6. Errores a Manejar

Agregar códigos de error en `lobby_protocol.h`:
```cpp
enum LobbyErrorCode : uint8_t {
    // ... existentes ...
    ERR_CAR_NOT_SELECTED = 0x09,      // Player ready sin seleccionar auto
    ERR_PLAYERS_NOT_READY = 0x0A,     // Iniciar sin todos listos
    ERR_INVALID_CAR_INDEX = 0x0B      // Índice de auto inválido
};
```

---

## 7. Sincronización (CRÍTICO)

### Waiting Room debe actualizar la UI con notificaciones:

Para esto, el `LobbyClient` necesita convertirse en `QObject` para emitir señales, o usar callbacks.

**Opción A: Convertir LobbyClient a QObject**
```cpp
class LobbyClient : public QObject {
    Q_OBJECT
signals:
    void playerJoinedNotification(uint16_t player_id, QString name, uint8_t car_index);
    void playerLeftNotification(uint16_t player_id);
    void playerReadyNotification(uint16_t player_id, bool is_ready);
    void carSelectedNotification(uint16_t player_id, uint8_t car_index);
    void gameStartedNotification(uint16_t game_id);
};

// En LobbyController, conectar señales:
connect(lobbyClient.get(), &LobbyClient::playerJoinedNotification,
        this, [this](uint16_t id, QString name, uint8_t car) {
            if (waitingRoomWindow) {
                waitingRoomWindow->addPlayer(name, getCarName(car), false);
            }
        });

connect(lobbyClient.get(), &LobbyClient::playerLeftNotification,
        this, [this](uint16_t id) {
            // TODO: Implementar removePlayer en WaitingRoomWindow
        });

connect(lobbyClient.get(), &LobbyClient::playerReadyNotification,
        this, [this](uint16_t id, bool ready) {
            if (waitingRoomWindow) {
                // TODO: Identificar índice del jugador y actualizar
                // waitingRoomWindow->setPlayerReady(index, ready);
            }
        });
```

**Opción B: Thread receptor en background**
Crear un thread que escuche notificaciones mientras está en waiting room y las despache al controller mediante señales Qt.

---

## 8. Diagrama de Estados del Jugador
```
┌─────────────┐
│   LOBBY     │
│  Principal  │
└──────┬──────┘
       │ [Jugar]
       ▼
┌─────────────┐
│   Ingreso   │
│   Nombre    │
└──────┬──────┘
       │ [Nombre confirmado]
       ▼
┌─────────────┐
│  Selección  │◄──────────┐
│   Partidas  │           │
└──┬───────┬──┘           │
   │       │              │
   │       └─[Crear]─────>┤
   │                      │
   │ [Unirse]             │
   ▼                      │
┌─────────────┐           │
│   Garage    │           │
│ (Selección  │<---------─┘
│    Auto)    │           
└──────┬──────┘           
       │ [Auto elegido]   
       ▼                  
┌─────────────┐           
│   Waiting   │           
│    Room     │───[Salir]─> se acaba o volvemos???
└──────┬──────┘
       │ [START_GAME]
       ▼
┌─────────────┐
│   Juego     │
│    (SDL)    │
└─────────────┘
```

---

## 9. Checklist de Implementación

### Protocolo:
- [ ] `MSG_SELECT_CAR` definido en enum
- [ ] `MSG_LEAVE_GAME` definido en enum  
- [ ] `MSG_PLAYER_READY` definido en enum
- [ ] `serialize_select_car()` implementado
- [ ] `serialize_leave_game()` implementado
- [ ] `serialize_player_ready()` implementado
- [ ] `serialize_start_game()` implementado correctamente

### Servidor:
- [ ] Estructura `PlayerInGame` con estado
- [ ] Handler para `MSG_SELECT_CAR`
- [ ] Handler para `MSG_PLAYER_READY`
- [ ] Handler para `MSG_START_GAME` con validaciones
- [ ] Handler para `MSG_LEAVE_GAME`
- [ ] Broadcast de notificaciones a jugadores en partida - Esto para luego
- [ ] Lógica de reasignación de host - No sé bien cómo era esto
- [ ] Validación "todos listos" antes de iniciar
- [ ] Transición a bucle de juego (SDL)

### Cliente:
- [ ] Implementar funciones placeholder en `lobby_client.cpp`
- [ ] Thread/loop receptor de notificaciones - Todavía estoy ciendo qué hago ocn esto, posible pregunta para ivan
- [ ] Conectar señales de notificación al controller
- [ ] Actualizar `WaitingRoomWindow` con datos reales - Fabi (cuando essté todo el protocolo completo)

### Testing:
- [ ] Test: Crear partida y esperar
- [ ] Test: Dos clientes en waiting room
- [ ] Test: Selección de autos sincronizada
- [ ] Test: Estados "ready" sincronizados
- [ ] Test: Host inicia partida
- [ ] Test: No-host intenta iniciar (debe fallar)
- [ ] Test: Abandonar partida desde garage
- [ ] Test: Abandonar partida desde waiting room
- [ ] Test: Reasignación de host al abandonar

---

## 10. Prioridades

### ✅ Completado:
- [x] Lobby visual completo
- [x] Flujo de login
- [x] Crear/unirse a partidas
- [x] Vista de garage
- [x] Vista de waiting room

### 🔴 Alta Prioridad (bloqueante para jugar):
1. **MSG_START_GAME** - Implementación completa en servidor
2. **Transición a SDL** - Cerrar Qt y abrir ventana SDL con juego
3. **Broadcast de GAME_STARTED** - Notificar a todos los clientes

### 🟡 Media Prioridad (mejora UX):
4. **MSG_PLAYER_READY** - Sincronización de estados
5. **Notificaciones JOIN/LEAVE** - Ver jugadores en tiempo real
6. **MSG_LEAVE_GAME** - Limpieza correcta al salir

### 🟢 Baja Prioridad (polish):
7. **MSG_SELECT_CAR** con notificación - Ver autos de otros
8. **Reasignación automática de host**
9. **Timeout de inactividad** - Expulsar jugadores AFK

---

## 11. Comandos de Prueba

### Probar el lobby completo:
```bash
# Terminal 1: Servidor
cd build
./server

# Terminal 2: Cliente 1
./client

# Terminal 3: Cliente 2 (opcional, para multi-jugador)
./client
```

### Flujo de prueba manual:
1. Cliente 1: Click en "Jugar"
2. Cliente 1: Ingresar nombre "Player1"
3. Cliente 1: Click en "Crear Nueva"
4. Cliente 1: Configurar partida (nombre, 2 jugadores, 3 carreras)
5. Cliente 1: Seleccionar auto
6. Cliente 2: Click en "Jugar"
7. Cliente 2: Ingresar nombre "Player2"
8. Cliente 2: Click en partida de Player1
9. Cliente 2: Seleccionar auto
10. Ambos: Click en "Listo!"
11. Cliente 1 (host): Click en "Iniciar!" → Debería empezar el juego

---

## 12. Estructura de Mensajes Pendientes

### MSG_SELECT_CAR (Cliente → Servidor)
```
[0x06][car_index]
1 byte: tipo de mensaje
1 byte: índice del auto (0-6)
```

### MSG_LEAVE_GAME (Cliente → Servidor)
```
[0x07][game_id_high][game_id_low]
1 byte: tipo de mensaje
2 bytes: ID de la partida (big-endian)
```

### MSG_PLAYER_READY (Cliente → Servidor)
```
[0x08][is_ready]
1 byte: tipo de mensaje
1 byte: 1 = listo, 0 = no listo
```

### MSG_PLAYER_JOINED_NOTIFICATION (Servidor → Clientes)
```
[0x15][player_id_high][player_id_low][name_len_high][name_len_low][...name...]
1 byte: tipo de mensaje
2 bytes: ID del jugador
2 bytes: longitud del nombre
N bytes: nombre del jugador (UTF-8)
```

### MSG_PLAYER_LEFT_NOTIFICATION (Servidor → Clientes)
```
[0x16][player_id_high][player_id_low]
1 byte: tipo de mensaje
2 bytes: ID del jugador que salió
```

### MSG_PLAYER_READY_NOTIFICATION (Servidor → Clientes)
```
[0x17][player_id_high][player_id_low][is_ready]
1 byte: tipo de mensaje
2 bytes: ID del jugador
1 byte: estado ready (1 = listo, 0 = no listo)
```

### MSG_CAR_SELECTED_NOTIFICATION (Servidor → Clientes)
```
[0x18][player_id_high][player_id_low][car_index]
1 byte: tipo de mensaje
2 bytes: ID del jugador
1 byte: índice del auto seleccionado
```

---

## 📞 Contacto con el Equipo de Servidor

**Antes de implementar**, coordinar con el equipo de servidor:

1. ✅ Confirmar que los tipos de mensaje no colisionen con otros sistemas
2. ✅ Validar el orden de bytes (big-endian para uint16_t)
3. ✅ Acordar manejo de errores y códigos de error
4. ✅ Definir quién implementa las funciones de serialización en `lobby_protocol.cpp`
5. ✅ Probar con herramientas de debugging (Wireshark, tcpdump) si hay problemas

---

## 🎯 Objetivo Final

**Estado deseado**: Dos o más clientes pueden:
1. Conectarse al servidor
2. Crear o unirse a una partida
3. Seleccionar sus autos
4. Marcar como "listos"
5. El host inicia el juego
6. Todos los clientes reciben `MSG_GAME_STARTED`
7. Se cierra Qt y se abre la ventana SDL con el juego sincronizado

---

## ⚠️ Notas Importantes

- **No modificar `lobby_protocol.h/cpp`** sin coordinación con el equipo de servidor
- **Todos los mensajes** deben usar **big-endian** para uint16_t
- **Los placeholders actuales** no rompen nada, solo imprimen logs
- **Testing incremental**: Implementar un mensaje a la vez y probar antes de continuar
- **Documentar cambios**: Actualizar este archivo cuando se implemente algo

---

