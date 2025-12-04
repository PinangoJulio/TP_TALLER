# Manual de Proyecto: Need for Speed 2D

**Materia:** Taller de Programación I (75.42) - FI-UBA
**Cuatrimestre:** 2C 2025
**Grupo:** [21]
**Repositorio:** [[Link al repo de Github](https://github.com/PinangoJulio/TP_TALLER)]

## 1. Integrantes y División de Tareas

Mayormente todos los integrantes participarion en cada una de las fases de la creacion e implementacion de juego pero principalmente la división de responsabilidades durante el desarrollo del Trabajo Práctico aproximadamente fue la siguiente .

| Integrante | Responsabilidades Principales |
|------------|-------------------------------|
| Lourdes De Meglio | Lógica del Servidor, Gameloop |
| Fabiola Romero |  Interfaz gráfica (Qt), mapas y checkpoints |
| Julianna Sanchez | Boxd2d, fisicas, implementacion de caminos |
| Julio Piñango | Lobby,reveiver, socket de clientes, shutdown del servidor  |

**Nota:** Aunque hubo tareas asignadas, todos los integrantes participaron en el debugging y en la integración final del código.

## 2. Cronograma y Organización

### Metodología de Trabajo
Para la organización semanal utilizamos Discord para comunicación diaria, github como controlador de versiones y manteniendo reuniones semanales presenciales/virtuales 

### Planificación vs. Realidad
El desarrollo se dividió en hitos (entregas). A continuación se compara lo planificado con lo sucedido:

- **(Hilo Aceptador / Sockets):** Se cumplió según lo estipulado. El mayor desafío fue entender el ciclo de vida de los sockets.
- **(Protocolo y Serialización):**  Se fue iterando el protocolo y la serializacion a medida que se fueran realizando las implmentaciones o nuevas funcionalidades que hicieron necesaria la necesidad de ir actualizandolo 
- **(Lógica del Juego y SDL):** Hubo una divergencia con el plan original respecto a prrgreso del tp, si bien en primera instancia se tenia definda las prioridades y responsabilidades, se tuvo que ir migrando y asignando mas personas en pro de continuar con el progreso del tp. 

## 3. Herramientas y Tecnologías

La elección de herramientas fue crucial para estandarizar el entorno de desarrollo entre los cuatro integrantes, quienes utilizaban diferentes sistemas operativos y preferencias de edición.

### Entorno de Desarrollo
- **Lenguaje Estándar:** C++17. Se eligió esta versión para aprovechar características modernas como `std::optional`, `std::variant`, y mejoras en el manejo de templates que simplificaron el código.
- **Sistema de Construcción:** CMake (versión mínima 3.10) y Make. CMake fue fundamental para abstraer la complejidad de los Makefiles y gestionar las dependencias de manera transparente.
- **IDEs utilizados:** El equipo fue agnóstico respecto al IDE.
  - **VS Code:** Usado por la mayoría por su ligereza y extensiones de CMake/C++.
  - **CLion:** Utilizado para sesiones de debugging profundo gracias a su integración superior con GDB.
  - **QtCreator:** Indispensable para el diseño visual de los archivos `.ui` del Lobby.
- **Control de Versiones:** Git y GitHub, utilizando GitHub Actions (si se implementó) para verificar la compilación en la nube.

### Librerías Externas
- **SDL2 (Simple DirectMedia Layer):** Incluyendo sus módulos SDL2_image (carga de PNGs), SDL2_mixer (audio multicanal) y SDL2_ttf (fuentes TrueType). Se eligió por su bajo nivel de abstracción, lo que permitió un control preciso sobre el render loop y el rendimiento gráfico.
- **Qt5:** Utilizada para toda la interfaz de usuario previa al juego (Lobby). Su sistema de Signals & Slots facilitó enormemente la lógica asincrónica de la red porejemplo para recibir lista de partidas sin congelar la UI.
- **YAML-CPP:** Librería robusta para el manejo de archivos de configuración y mapas. Nos permitió tener archivos legibles , facilitando la depuración manual de los niveles.
- **Google Test:** Framework utilizado para las pruebas unitarias. Fue crítico para testear la lógica del protocolo (serialización/deserialización) sin necesidad de levantar el juego completo.

### Calidad de Código (Linters)
Para evitar discusiones sobre estilo y prevenir errores comunes, configuramos un entorno estricto de calidad de código:
- **CPPLINT:** Configurado para seguir la guía de estilo de Google. Nos ayudó a mantener consistencia en nombres de variables, inclusiones de cabeceras y comentarios.
- **Clang-Format:** Automatizó el formateo (indentación, llaves, espacios). Se ejecutaba automáticamente, eliminando commits de "fix indentation".
- **Pre-commit:** Framework en Python que orquestaba estos chequeos localmente antes de permitir un commit, asegurando que nada "sucio" llegara al repositorio remoto.

### Documentación y Recursos de Aprendizaje
El aprendizaje autodidacta fue constante. Nuestras fuentes principales fueron:
- **SDL2 Wiki & Lazy Foo' Productions:** La "biblia" para entender SDL. Los tutoriales de Lazy Foo fueron la base para nuestra clase de texturas y manejo de sprites.
- **Qt Documentation:** La documentación oficial de Qt es excelente para entender la jerarquía de clases de QWidget y el ciclo de vida de QApplication.
- **CppReference:** Consulta diaria para detalles sobre la STL, especialmente sobre el uso correcto de punteros inteligentes (`std::unique_ptr`, `std::shared_ptr`) y contenedores.
- **Beej's Guide to Network Programming:** Referencia clave para la implementación de Sockets. Nos ayudó a entender la diferencia entre sockets bloqueantes y no bloqueantes, y cómo manejar el cierre de conexiones (TCP FIN/RST).

## 4. Puntos Problemáticos y Desafíos

Durante el desarrollo, los mayores obstáculos técnicos fueron:

1. **Integración Qt + SDL2:** [Sugerencia de redacción] Lograr que la ventana de Qt (Lobby) lanzara correctamente la ventana de SDL (Juego) sin conflictos de contexto o hilos fue complejo. Se resolvió mediante [explicar brevemente: ej. cerrando la app Qt antes de abrir SDL o manejando procesos separados].
2. **Manejo de Concurrencia (Race Conditions):** [Sugerencia] Coordinar el Gameloop del servidor con el envío de mensajes a múltiples clientes requirió un uso cuidadoso de mutex y colas protegidas para evitar race conditions.
3. **Actualizacion en vivo del LObby:** Lograr que loc clientes pudieran ver cambios en vivo de la lista de jugadores, de la cantidad de jugadores que iban ingresando, como seleccionaban el auto y cuando le dieran click al listo, todos esos cambios en vivo fueron complejos de realizar
4. **Sistema de Coordenadas:** Transformar las coordenadas lógicas del servidor a las coordenadas de píxeles en pantalla (SDL) trajo confusiones iniciales, especialmente con el sistema de cámara/viewport.

## 5. Errores Conocidos

Al momento de la entrega final, persisten los siguientes comportamientos no deseados o limitaciones:

- Ciertos puentes no pueden ser cruzados por los autos.
- Cuando se abren 2 partidas hay que darle click al otro mapa para que pueda renderizar al 2do cliente y pueda ejecutarse normalmente
- Existe un bug en la transicion entre 2 partidas cuando se le da click a la 'w' para activar el cheat de ganar carrera pero que no limita el juego.
- El auto parece ir drifteando a veces, no se posiciona en la direccion que deberia cuando va moviendose.

## 6. Conclusiones y Retroalimentación

### Si volviéramos a empezar
Si tuviéramos que rehacer el proyecto desde cero con el conocimiento actual, haríamos los siguientes cambios:

- Abstraeríamos mejor la clase Protocolo para que no dependa tanto del modelo del juego. 
- Usaríamos una máquina de estados más formal para el cliente
- Implementar el box2D de una manera mas temprana
- Asignar mas personas a el gameloop para poder avanzar mas rapido.
- Definiríamos los DTOs (Data Transfer Objects) el primer día para evitar desacuerdos entre Cliente y Servidor.

### Sugerencias para la Cátedra
Creemos que la cursada podría mejorar si se incluyera:

- Mayor cantidad de ejemplos hechos del multithreads.
- Más ejemplos prácticos sobre el patrón Game Loop en la teoría.
- Documentación oficial sobre cómo estructurar el polimorfismo en los paquetes de red.