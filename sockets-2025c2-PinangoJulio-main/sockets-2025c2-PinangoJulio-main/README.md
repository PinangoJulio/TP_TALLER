# TP Sockets - Need for Speed PoC

## Descripción

Este proyecto implementa una prueba de concepto (PoC) para la fase de selección de autos del juego Need for Speed. El sistema permite a un cliente conectarse a un servidor para consultar autos disponibles, ver el auto actual y realizar compras.


## Estructura del Proyecto

```
├── client_src/
│   ├── client.h
│   ├── client.cpp
│   └── client_main.cpp
├── server_src/
│   ├── server.h
│   ├── server.cpp
│   └── server_main.cpp
├── common_src/
│   ├── common_protocol.h
│   ├── common_protocol.cpp
│   ├── common_constants.h
│   ├── common_socket.*
│   ├── resolver.*
│   ├── liberror.*
│   └── resolvererror.*
├── MakefileSockets
└── README.md
```
##  Implementación Realizada
- **Socket y comunicación de red**: Implementación provista por la cátedra
  - Fuente: https://github.com/eldipa/sockets-en-cpp
  - Licencia: Ver repositorio original
- **Protocolo, Cliente y Servidor**
- **Diseño de DTOs y arquitectura**

