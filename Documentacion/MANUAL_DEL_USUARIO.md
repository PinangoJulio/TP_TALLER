# Need for Speed 2D - Manual de Usuario


**Versión:** 1.0  
**Fecha:** Diciembre 2025  
**Sistema Operativo:** Ubuntu 24.04 LTS (Xubuntu 24.04)

---

## Tabla de Contenidos

1. [Introducción](#introducción)
2. [Requisitos del Sistema](#requisitos-del-sistema)
3. [Instalación](#instalación)
4. [Configuración](#configuración)
5. [Cómo Jugar](#cómo-jugar)
7. [Solución de Problemas](#solución-de-problemas)
8. [Preguntas Frecuentes](#preguntas-frecuentes)

---

## Introducción

**Need for Speed 2D** es un juego de carreras multijugador en 2D inspirado en el clásico Need for Speed. Compite contra hasta 8 jugadores en carreras por 3 ciudades icónicas del Grand Theft Auto: **Liberty City**, **San Andreas** y **Vice City**.

### Video Promocional 

Link al video: https://www.youtube.com/watch?v=_z2H5_P65eg

### Características Principales del juego

- **3 Ciudades únicas** con múltiples circuitos
- **Variedad de autos** con diferentes características
- **Multijugador** hasta 8 jugadores simultáneos
- **Música**
- **Sistema de rankings** por carrera y total

---

## Requisitos del Sistema

### Sistema Operativo
- **Ubuntu 24.04 LTS** o **Xubuntu 24.04 LTS**

### Dependencias del Sistema

El proyecto requiere las siguientes bibliotecas y herramientas:

#### Herramientas de Compilación
```bash
- cmake >= 3.28
- g++ >= 13.0 (con soporte C++20)
- make
- git
```

#### Bibliotecas de Desarrollo
```bash
- libsdl2-dev >= 2.0.22
- libsdl2-image-dev >= 2.0.5
- libsdl2-mixer-dev >= 2.0.4
- libsdl2-ttf-dev >= 2.0.18
- qt6-base-dev >= 6.4.0
- qt6-multimedia-dev >= 6.4.0
- libyaml-cpp-dev >= 0.7.0
- libbox2d-dev >= 2.4.0
```

#### Bibliotecas del Sistema
```bash
- libasound2
- libpulse0
- libxcursor1
- libxrandr2
- libxi6
- libgl1
- libxmp4
- libfluidsynth3
- libwavpack1
- libopusfile0
```

---

## Instalación

###  Instalación Automática (Recomendado)

El proyecto incluye un **instalador automático** que se encarga de todo el proceso.

#### Paso 1: Clonar el Repositorio

```bash
# Abrir una terminal
cd ~/Escritorio
git clone https://github.com/tu_usuario/TP_TALLER.git
cd TP_TALLER
```

#### Paso 2: Ejecutar el Instalador

```bash
sudo make install
```
o en su defecto: 

```bash
./install.sh
```

Este comando realiza automáticamente:
1. Corrige permisos de carpetas
2. Configura el proyecto con CMake
3. Compila todos los componentes (servidor, cliente, editor)
4. Ejecuta los tests unitarios
5. Instala los binarios en el sistema

**Tiempo estimado:** 5-10 minutos (puede tomar un tiempo, dependiendo del hardware de la computadora donde se instale)

#### Paso 2: Verificar la Instalación

```bash
# Verificar que los ejecutables se crearon
ls -lh client server taller_editor
```

Deberías ver algo como:
```
-rwxr-xr-x 1 usuario usuario 2.1M dic  3 10:00 client
-rwxr-xr-x 1 usuario usuario 1.8M dic  3 10:00 server
-rwxr-xr-x 1 usuario usuario 1.5M dic  3 10:00 taller_editor
```

### Paso 3: Compilar el codigo

```bash
make debug

```

---


#### Paso 4: Ejecutar Tests

```bash
./taller_tests
```

---

## ⚙Configuración

### Estructura de Archivos

Después de la instalación, el proyecto tiene la siguiente estructura:

```
TP_TALLER/
├── client              # Ejecutable del cliente
├── server              # Ejecutable del servidor
├── taller_editor       # Ejecutable del editor
├── config.yaml         # Configuración principal del servidor
├── assets/             # Recursos del juego
│   ├── fonts/          # Fuentes
│   ├── img/            # Imágenes y sprites
│   │   ├── map/        # Mapas de las ciudades
│   │   └── lobby/      # Imágenes del lobby
│   └── music/          # Música de fondo
└── server_src/
    └── city_maps/      # Configuración de ciudades y rutas
        ├── Liberty City/
        │   ├── ruta-1.yaml
        │   ├── ruta-2.yaml
        │   └── ruta-3.yaml
        ├── San Andreas/
        │   ├── ruta-1.yaml
        │   ├── ruta-2.yaml
        │   └── ruta-3.yaml
        └── Vice City/
            ├── ruta-1.yaml
            ├── ruta-2.yaml
            └── ruta-3.yaml
```

### Archivo de Configuración Principal

El archivo `config.yaml` contiene la configuración del servidor:

```yaml
server:
  port: 8080
  max_players: 8
  
game:
  max_race_time_ms: 600000 
  countdown_seconds: 3
  
cars:
  - name: "Turbo GT"
    type: "sport"
    max_speed: 200.0
    acceleration: 50.0
    health: 80.0
```

**Importante:** No es necesario modificar este archivo para uso normal. Solo edítalo si quieres ajustar parámetros del juego.

### Configuración de Red

#### Para Jugar en la Misma Computadora
No necesitas configurar nada. Usa `localhost` o `127.0.0.1`.


## Cómo Jugar

### Paso 1: Iniciar el Servidor

Abre una terminal y ejecuta:

```bash
cd ~/Escritorio/TP_TALLER
./server config.yaml
```

El servidor está listo a la espera de conexiones. En el yaml se especifica el puerto determinado 8080.

**Consejo:** Mantén esta terminal abierta. Para detener el servidor, presiona `q` y Enter.

---

### Paso 2: Iniciar el Cliente

En una **nueva terminal** ejecuta:

```bash
cd ~/Escritorio/TP_TALLER
./client
```

Se conectará al servidor y abrirá la ventana del **Lobby**:

---

### Paso 3: Conectarse al Servidor

1. **Ingresa tu nombre de usuario** (ej: "Jugador1")
2. Click en **"Conectar"**
3. Si la conexión es exitosa, verás la lista de partidas disponibles


---

### Paso 4: Crear o Unirse a una Partida

#### Opción A: Crear Nueva Partida

1. Click en **"Crear Partida"**
2. Completa los datos:
    - **Nombre de la partida:** Ej: "Carrera Rápida"
    - **Máximo de jugadores:** Entre 2 y 8
    - **Seleccionar carreras:** Elige las ciudades y rutas, por ejemplo:
        - Liberty City - Ruta 1
        - San Andreas - Ruta 2
        - Vice City - Ruta 3
3. Click en **"Crear"**


#### Opción B: Unirse a Partida Existente

1. En la lista de partidas, selecciona una. Aprete 'actualizar' por si hubiera nuevas partidas
2. Click en **"Unirse"**
3. Espera a que el host inicie la partida


---

### Paso 5: Seleccionar tu Auto

Una vez en la sala de espera:

1. Click en **"Garage"**
2. Examina las estadísticas de cada auto:
    - **Sport:** Rápido, frágil
    - **Sedan:** Balanceado
    - **Truck:** Lento, resistente
    - **Bike:** Muy rápido, muy frágil
3. Click en el auto que prefieras
4. Click en **"Seleccionar"**
5. Marca **"Listo"** cuando estés preparado


**Importante:** Una vez seleccionado, usarás ese auto para **todas las carreras** de la partida.

---

### Paso 6: ¡A Correr!

Cuando todos los jugadores estén listos, el host puede iniciar la partida.

#### Controles del Juego

**Movimiento:**
- **↑ (Flecha Arriba):** Acelerar hacia arriba
- **↓ (Flecha Abajo):** Acelerar hacia abajo
- **← (Flecha Izquierda):** Acelerar hacia la izquierda
- **→ (Flecha Derecha):** Acelerar hacia la derecha

**Power-ups:**
- **Espacio:** Usar Nitro, aumenta velocidad temporalmente

**Cheats:**
- **W:** Ganar la carrera automáticamente

**Otros:**
- **Esc:** Salir de la carrera y terminar la partida para ese juador


└────────────────────────────────────────────┘

** Elementos en la pantalla **

**Minimapa (esquina inferior derecha):**
- Se puede visualizar el camino marcado con los *hints* que debe seguir el jugador en esa carrera. 
- Además se observa un punto azul que indica la posición actual del jugador en el mapa, y se mueve en tiempo real conforme avanza en la carrera.

**Checkpoints** 
- Son rectangulos visuales que marcan el camino de una carrera que los jugadores deben seguir.
- El checkpoint inicial es de color verde y el que marca el final de la carrera rojo, los demás son intermedios y necesarios para guiar

---

### Paso 7: Finalizar la Carrera

Debes cruzar **todos los checkpoints** en orden hasta llegar al **checkpoint FINISH** (En rojo).

#### Cuando Terminas una Carrera

1. Esperas a que todos terminen

2. Después de 5 segundos, automáticamente pasa a la siguiente carrera, los autos aparecen en sus posiciones iniciales correspondientes.

#### Ranking Final

Después de completar todas las carreras, se muestra el **Ranking Total**:

---


## Solución de Problemas


---

**Error:** `fatal error: SDL2/SDL.h: No such file or directory`

**Solución:** Instalar dependencias SDL
```bash
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev
```

---

**Error:** `fatal error: yaml-cpp/yaml.h: No such file or directory`

**Solución:**
```bash
sudo apt install libyaml-cpp-dev
```

---


### El juego va muy lento / se traba

**Soluciones:**
1. Cierra otras aplicaciones
2. Reduce la calidad gráfica (si está implementado)
3. Verifica tu GPU:
   ```bash
   glxinfo | grep "OpenGL version"
   ```
4. Actualiza drivers gráficos:
   ```bash
   sudo ubuntu-drivers autoinstall
   ```


## Algunas Preguntas Frecuentes que pueden surgirle al usuario

### ¿Cuántos jugadores pueden jugar simultáneamente?
Hasta **8 jugadores** en la misma partida.

### ¿Los cambios en el editor afectan al servidor inmediatamente?
Sí, pero debes **reiniciar el servidor** para que cargue los cambios.

### ¿Puedo usar cheats en partidas normales?
Los cheats están diseñados para **pruebas**. Usarlos en partidas multijugador puede arruinar la experiencia de otros jugadores.

### ¿Qué pasa si un jugador se desconecta?
Su auto queda inmovilizado (como si estuviese sin vida) y la carrera continúa con los jugadores restantes.

### ¿Cuánto dura una carrera?
Depende del circuito y los jugadores, pero hay un **límite de 10 minutos** por carrera. Si se alcanza, la carrera termina automáticamente.

### ¿Puedo cambiar de auto entre carreras?
No, el auto seleccionado se mantiene para **toda la partida**.

---



### Desarrolladores
Este proyecto fue desarrollado por:
- Nombre 1 -Lourdes De Meglio
- Nombre 2 - Fabiola Romero
- Nombre 3 - Julianna Sanchez
- Nombre 4 - Julio Piñango 

---


Este proyecto es software educativo desarrollado para la materia Taller de Programación I (75.42) de FIUBA.

---

## ¡Disfruta el Juego!

Esperamos que disfrutes **Need for Speed 2D**. ¡Nos vemos en las carreras!

---

**Última actualización:** Diciembre 2025
**Versión del Manual:** 1.0
