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

add_executable(lobby_client_test
    lobby/lobby_client.cpp      # Lógica del cliente
    lobby/lobby_client.h        # Header
    ${CMAKE_SOURCE_DIR}/tests/lobby_integration_test.cpp  
)

target_include_directories(lobby_client_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}  
)

target_link_libraries(lobby_client_test
    PRIVATE
    taller_common
)

set_target_properties(lobby_client_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)