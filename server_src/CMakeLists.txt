target_sources(server
    PRIVATE
    # .cpp files
    main.cpp
    server_main.cpp
    server.cpp
    server_acceptor.cpp
    server_client_handler.cpp
    server_monitor.cpp
    server_receiver.cpp
    server_sender.cpp
    server_protocol.cpp
    server_game.cpp
    server_car.cpp
    
    PUBLIC
    # .h files
    server.h
    server_acceptor.h
    server_client_handler.h
    server_monitor.h
    server_receiver.h
    server_sender.h
    server_protocol.h
    server_game.h
    server_car.h

)

# TEMPORAL: Ejecutable de prueba del lobby
add_executable(lobby_server_test
    lobby/lobby_main.cpp
)

# ✅ Linkear con taller_lobby
target_link_libraries(lobby_server_test
    PRIVATE
    taller_common
    taller_lobby  # NUEVO
)

set_target_properties(lobby_server_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)