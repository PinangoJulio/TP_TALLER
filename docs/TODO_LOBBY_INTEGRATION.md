# TODO: Adaptación del Lobby para Client::start()

## Problema Actual

En el modelo del ejemplo que me pasaste, el lobby funciona así:

```cpp
void Client::iniciar() {
    LobbyWindow lobby(protocolo, username);
    int result = lobby.exec();  // ← Bloquea hasta que termine
    
    if (result != QDialog::Accepted || !lobby.lobbySalioBien()) {
        protocolo.enviar_salir_lobby();
        return;  // ← NO continuar con el juego
    }
    
    // ✅ Lobby exitoso → continuar con threads y SDL
    std::string mapa_inicial = protocolo.recibir_mapa();
    // ...
}
```

**Características clave:**
1. `lobby.exec()` **bloquea** hasta que el usuario termine el lobby
2. Devuelve un **código de resultado** (`QDialog::Accepted` o `QDialog::Rejected`)
3. Tiene método `lobbySalioBien()` para verificar si está todo listo
4. Si sale mal, **NO continúa** con el juego

## Solución Propuesta

Hay 2 opciones para adaptar tu código:

### **Opción 1: Crear LobbyManager (Recomendada)**

Crear una clase `LobbyManager` que encapsule todo el flujo del lobby y funcione como `LobbyWindow` del ejemplo:

```cpp
// lobby/lobby_manager.h
class LobbyManager : public QDialog {
    Q_OBJECT
    
private:
    LobbyController* controller;
    bool lobby_successful;
    uint16_t player_id;
    std::string selected_car;
    uint16_t match_id;
    
public:
    LobbyManager(const char* host, const char* port, const char* username);
    
    // Ejecutar lobby (bloquea hasta terminar)
    int exec() override;
    
    // Verificar si salió bien
    bool is_ready_to_race() const { return lobby_successful; }
    
    // Obtener datos
    uint16_t get_player_id() const { return player_id; }
    std::string get_selected_car() const { return selected_car; }
    uint16_t get_match_id() const { return match_id; }
};
```

**Uso en Client::start():**
```cpp
void Client::start() {
    // Crear lobby manager
    LobbyManager lobby(protocol.get_host().c_str(), 
                      protocol.get_port().c_str(),
                      username.c_str());
    
    // Ejecutar lobby (bloquea hasta terminar)
    int result = lobby.exec();
    
    // Verificar resultado
    if (result != QDialog::Accepted || !lobby.is_ready_to_race()) {
        std::cout << "[Client] ❌ Lobby cancelado o falló" << std::endl;
        protocol.disconnect();
        return;  // ← NO continuar
    }
    
    // ✅ Lobby exitoso → obtener datos
    player_id = lobby.get_player_id();
    std::string selected_car = lobby.get_selected_car();
    uint16_t match_id = lobby.get_match_id();
    
    std::cout << "[Client] ✅ Lobby completado: player_id=" << player_id 
              << ", car=" << selected_car << std::endl;
    
    // Continuar con threads y SDL...
    sender.start();
    receiver.start();
    // ...
}
```

---

### **Opción 2: Modificar LobbyController existente**

Añadir métodos a `LobbyController` para que funcione de forma similar:

```cpp
// lobby_controller.h
class LobbyController : public QObject {
    // ...existing code...
    
private:
    bool lobby_completed_successfully;
    bool ready_to_race;
    
public:
    // Ejecutar lobby y esperar resultado (bloqueante)
    bool run_lobby_and_wait();
    
    // Verificar estado
    bool is_ready_to_race() const { return ready_to_race; }
    
    // Obtener datos
    uint16_t get_player_id() const;
    std::string get_selected_car() const;
    uint16_t get_match_id() const { return currentGameId; }
    
signals:
    void lobby_finished(bool success);  // Señal cuando termina
};
```

**Implementación:**
```cpp
bool LobbyController::run_lobby_and_wait() {
    ready_to_race = false;
    
    // Iniciar lobby
    start();
    
    // Crear event loop local para bloquear
    QEventLoop loop;
    connect(this, &LobbyController::lobby_finished, &loop, &QEventLoop::quit);
    
    // Esperar hasta que termine
    loop.exec();  // ← Bloquea aquí
    
    return ready_to_race;
}
```

**Uso en Client::start():**
```cpp
void Client::start() {
    LobbyController lobby(protocol.get_host().c_str(), 
                         protocol.get_port().c_str());
    
    // Ejecutar y esperar (bloqueante)
    if (!lobby.run_lobby_and_wait()) {
        std::cout << "[Client] ❌ Lobby cancelado" << std::endl;
        protocol.disconnect();
        return;
    }
    
    // ✅ Lobby exitoso
    player_id = lobby.get_player_id();
    std::string selected_car = lobby.get_selected_car();
    
    // Continuar...
}
```

---

## Cambios Necesarios en LobbyController

Para que cualquiera de las opciones funcione, necesitas:

1. **Detectar cuando el usuario aprieta "Iniciar"**:
   - En `WaitingRoomWindow`, cuando el host aprieta "Iniciar carrera"
   - Emitir señal `lobby_finished(true)`

2. **Detectar cuando el usuario cancela**:
   - Si cierra cualquier ventana sin completar el flujo
   - Si sale de la partida
   - Emitir señal `lobby_finished(false)`

3. **Guardar datos importantes**:
   ```cpp
   private:
       uint16_t player_id;        // ID asignado por servidor
       std::string selected_car;  // Auto elegido
       uint16_t match_id;         // ID de la partida
       bool ready_to_race;        // ¿Listo para correr?
   ```

4. **Métodos getters**:
   ```cpp
   uint16_t get_player_id() const { return player_id; }
   std::string get_selected_car() const { return selected_car; }
   uint16_t get_match_id() const { return match_id; }
   bool is_ready_to_race() const { return ready_to_race; }
   ```

---

## Ejemplo: Detectar "Iniciar Carrera"

En `WaitingRoomWindow::on_start_button_clicked()`:

```cpp
void WaitingRoomWindow::on_start_button_clicked() {
    // Enviar comando al servidor
    emit start_game_requested();
    
    // Notificar al controller que el lobby terminó exitosamente
    emit lobby_ready_to_race();  // ← Nueva señal
}
```

En `LobbyController`:

```cpp
void LobbyController::onStartGameRequested() {
    // Enviar start al servidor
    lobbyClient->start_game(currentGameId);
    
    // Marcar como listo
    ready_to_race = true;
    
    // Cerrar todas las ventanas del lobby
    if (waitingRoomWindow) waitingRoomWindow->close();
    if (matchSelectionWindow) matchSelectionWindow->close();
    
    // Emitir señal de finalización
    emit lobby_finished(true);  // ← Desbloquea el exec()
}
```

---

## Implementación Mínima

Si no quieres cambiar mucho, puedes hacer esto:

**En `lobby_controller.h`:**
```cpp
class LobbyController : public QObject {
    // ...existing code...
    
private:
    bool ready_to_race = false;
    
public:
    bool is_ready_to_race() const { return ready_to_race; }
    uint16_t get_player_id() const;  // TODO: implementar
    
signals:
    void lobby_finished(bool success);
};
```

**En `Client::start()`:**
```cpp
void Client::start() {
    LobbyController lobby(protocol.get_host().c_str(), 
                         protocol.get_port().c_str());
    
    QEventLoop loop;
    QObject::connect(&lobby, &LobbyController::lobby_finished, 
                     &loop, &QEventLoop::quit);
    
    lobby.start();
    loop.exec();  // ← Bloquea hasta lobby_finished()
    
    if (!lobby.is_ready_to_race()) {
        std::cout << "[Client] Lobby cancelado" << std::endl;
        return;
    }
    
    // ✅ Continuar con threads y SDL...
}
```

---

## Conclusión

La **Opción 1** (crear `LobbyManager`) es más limpia y separa responsabilidades.
La **Opción 2** (modificar `LobbyController`) es más rápida si no quieres refactorizar mucho.

Ambas logran el mismo objetivo: que `Client::start()` **espere** a que el lobby termine antes de continuar con los threads y SDL, igual que en el ejemplo que me pasaste.

