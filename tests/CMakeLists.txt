target_sources(taller_tests
    PRIVATE
    # .cpp files de tests
    protocol_tests.cpp
    lobby_tests.cpp
    physics_tests.cpp
    collision_tests.cpp
    
    ${CMAKE_SOURCE_DIR}/server_src/game/game_loop.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/car.cpp
    ${CMAKE_SOURCE_DIR}/server_src/network/monitor.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/collision_handler.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/obstacle.cpp 
    
    PUBLIC
    # .h files
)

# Incluir directorios necesarios
target_include_directories(taller_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/server_src/lobby
    ${CMAKE_SOURCE_DIR}
)