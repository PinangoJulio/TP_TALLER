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
    game/game_state.cpp
    game/car.cpp
    game/physics/physics_world.cpp
    
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
    game/game_state.h
    game/car.h
    game/physics/physics_world.h
    network/client_handler.h
    network/receiver.h
    network/sender.h
    network/monitor.h
)

# Ejecutable temporal del lobby (para testing)
add_executable(lobby_server_test
    lobby/lobby_main.cpp
)

target_link_libraries(lobby_server_test
    PRIVATE
    taller_common
    taller_lobby
)

set_target_properties(lobby_server_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)