target_sources(taller_tests
    PRIVATE
    # .cpp files
    protocol_tests.cpp
    foo.cpp
    lobby_tests.cpp  # NUEVO
    
    PUBLIC
    # .h files
)

# Agregar dependencias necesarias para lobby_tests
target_include_directories(taller_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/server_src/lobby
)