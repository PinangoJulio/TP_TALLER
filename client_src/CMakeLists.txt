target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    client.cpp
    client_protocol.cpp
    lobby_window.cpp
    lobby_client.cpp  
    
    PUBLIC
    # .h files
    client.h
    client_protocol.h
    lobby_window.h
    lobby_client.h
)

# TEMPORAL: Ejecutable de prueba del lobby
add_executable(lobby_client_test
    lobby_test_client.cpp
    lobby_client.cpp
)

target_link_libraries(lobby_client_test
    PRIVATE
    taller_common
)

# Mover el ejecutable a la raíz
set_target_properties(lobby_client_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)