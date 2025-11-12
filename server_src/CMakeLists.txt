target_sources(server
    PRIVATE
    # Main
    main.cpp
    server.cpp
    acceptor.cpp
    
    # Lobby
    lobby/lobby_server.cpp
    lobby/lobby_manager.cpp
    lobby/game_room.cpp
    
    # Game
    game/game_loop.cpp
    game/car.cpp
    
    # Network
    network/client_handler.cpp
    network/receiver.cpp
    network/sender.cpp
    network/monitor.cpp
    
    PUBLIC
    # Headers
    server.h
    acceptor.h
    lobby/lobby_server.h
    lobby/lobby_manager.h
    lobby/game_room.h
    game/game_loop.h
    game/car.h
    network/client_handler.h
    network/receiver.h
    network/sender.h
    network/monitor.h
)

# Ejecutable temporal del lobby (para testing)
