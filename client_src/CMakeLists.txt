target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    lobby/lobby_window.cpp
    lobby/lobby_client.cpp  
    
    PUBLIC
    # .h files
    lobby/lobby_window.h
    lobby/lobby_client.h
)

# TEMPORAL: Ejecutable de prueba del lobby
add_executable(lobby_client_test
    lobby/lobby_client.cpp
)

target_link_libraries(lobby_client_test
    PRIVATE
    taller_common
)

# Mover el ejecutable a la raíz
set_target_properties(lobby_client_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)