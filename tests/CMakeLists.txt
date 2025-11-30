target_sources(taller_tests
    PRIVATE
    # --- TESTS ---
    lobby_tests.cpp
    physics_tests.cpp
    collision_tests.cpp
    map_loader_tests.cpp
    physics_advanced_tests.cpp
    
    # --- GAME LOGIC ---
    ${CMAKE_SOURCE_DIR}/server_src/game/game_loop.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/car.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/collision_handler.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/obstacle.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/map_loader.cpp
    ${CMAKE_SOURCE_DIR}/server_src/game/checkpoint_manager.cpp
    
    # --- DEPENDENCIAS DE GAME LOOP (Faltaban estas) ---
    ${CMAKE_SOURCE_DIR}/server_src/network/monitor.cpp
    ${CMAKE_SOURCE_DIR}/server_src/network/client_monitor.cpp
    
    # --- COMMON (Si no linkeas la librería 'taller_common', necesitas sus fuentes) ---
    ${CMAKE_SOURCE_DIR}/common_src/config.cpp
    ${CMAKE_SOURCE_DIR}/common_src/game_state.cpp
    # (Agrega otros de common si te dan error de linker)
)

# Asegurarse de linkear librerías externas
target_link_libraries(taller_tests
    PRIVATE
    GTest::gtest_main
    box2d::box2d        # Necesario para la física
    taller_common       # Si tienes esta librería definida, úsala y borra los sources de common de arriba
    yaml-cpp            # Necesario para MapLoader/Config
)

target_include_directories(taller_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/server_src/lobby
    ${CMAKE_SOURCE_DIR}
)