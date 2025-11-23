cmake_minimum_required(VERSION 3.24)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS ON)

project(
  taller_tp
  VERSION 1.0
  HOMEPAGE_URL "https://github.com/URL-HERE"
  LANGUAGES CXX)

# Opciones del proyecto
option(TALLER_TESTS "Enable / disable tests." ON)
option(TALLER_CLIENT "Enable / disable client program." ON)
option(TALLER_SERVER "Enable / disable server program." ON)
option(TALLER_EDITOR "Enable / disable editor program." ON)
option(TALLER_MAKE_WARNINGS_AS_ERRORS "Enable / disable warnings as errors." ON)

message(CMAKE_CXX_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")

# 1. ENCONTRAR E IMPORTAR QT
find_package(
  Qt6
  COMPONENTS Widgets Multimedia
  REQUIRED)

# 1. HABILITAR EL COMPILADOR META-OBJETO (MOC)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# =======================================================
# Dependencias Externas (FetchContent)
# =======================================================

include(FetchContent)

# --- SDL y SDL2pp (para cliente/editor) ---
if(TALLER_CLIENT OR TALLER_EDITOR)
  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.8
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    OVERRIDE_FIND_PACKAGE)
  FetchContent_Declare(
    SDL2_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG release-2.8.2
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    OVERRIDE_FIND_PACKAGE)
  FetchContent_Declare(
    SDL2_mixer
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
    GIT_TAG release-2.8.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    OVERRIDE_FIND_PACKAGE)
  FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-2.22.0
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    OVERRIDE_FIND_PACKAGE)
  FetchContent_Declare(
    libSDL2pp
    OVERRIDE_FIND_PACKAGE
    URL https://github.com/libSDL2pp/libSDL2pp/archive/cc198c9a5657048bee67ece82de620b2d5661084.zip
  )
  FetchContent_MakeAvailable(SDL2 SDL2_image SDL2_mixer SDL2_ttf libSDL2pp)
endif()

# --- Box2D Integration (IMPORTANTE: Usar nombre en minúsculas) ---
FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v3.1.1
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(box2d)

# --- yaml-cpp Integration (Parser YAML) ---
# Primero intentar usar el paquete del sistema; si no existe, bajar un tarball
# estable (no git).
find_package(yaml-cpp QUIET)

if(NOT yaml-cpp_FOUND AND NOT TARGET yaml-cpp)
  include(FetchContent)
  FetchContent_Declare(
    yaml-cpp
    URL https://github.com/jbeder/yaml-cpp/archive/refs/tags/yaml-cpp-0.7.0.tar.gz
    # URL_HASH SHA256=<<opcional-agregar-sha256-para-integridad>>
  )
  FetchContent_MakeAvailable(yaml-cpp)
endif()

# Si el paquete del sistema exporta yaml-cpp::yaml-cpp, crear un alias simple
# "yaml-cpp"
if(TARGET yaml-cpp::yaml-cpp AND NOT TARGET yaml-cpp)
  add_library(yaml-cpp ALIAS yaml-cpp::yaml-cpp)
endif()

# --- GoogleTest (para tests) ---
if(TALLER_TESTS)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  set(gtest_force_shared_crt
      ON
      CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  include(GoogleTest)
endif()

# =======================================================
# Librería Común (taller_common)
# =======================================================

add_library(taller_common STATIC common_src/config.h common_src/config.cpp
                                 common_src/general_paths.h)
add_subdirectory(common_src)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(taller_common ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)
target_include_directories(taller_common PUBLIC .)

# Box2D está fuera para evitar propagación al cliente. Usar el alias yaml-cpp
# (ya sea del sistema o del FetchContent)
target_link_libraries(taller_common PUBLIC yaml-cpp)

# =======================================================
# Librería de Lobby (compartida entre server y tests)
# =======================================================

add_library(
  taller_lobby STATIC
  server_src/lobby/lobby_manager.cpp server_src/lobby/game_room.cpp
  server_src/lobby/lobby_server.cpp)

target_include_directories(taller_lobby PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(taller_lobby PUBLIC taller_common)

set_project_warnings(taller_lobby ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

# =======================================================
# Program section (Ejecutables) - UNA SOLA VEZ
# =======================================================

# --- CLIENTE ---
# --- CLIENTE ---
if(TALLER_CLIENT)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

  add_executable(client)

  add_subdirectory(client_src) # PRIMERO el subdirectorio

  # DESPUÉS los includes (AMBAS líneas necesarias)
  target_include_directories(client PRIVATE ${libSDL2pp_SOURCE_DIR}/include
                                            ${CMAKE_SOURCE_DIR})

  set_project_warnings(client ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

  target_link_libraries(
    client
    PRIVATE taller_common
            SDL2pp::SDL2pp
            SDL2::SDL2
            SDL2_image::SDL2_image
            SDL2_ttf::SDL2_ttf
            SDL2_mixer::SDL2_mixer
            Qt6::Widgets
            Qt6::Multimedia
            Qt6::Core
            Qt6::Network)
endif()
# --- SERVIDOR ---
if(TALLER_SERVER)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
  add_executable(
    server
    server_src/network/matches_monitor.h
    server_src/game/player.h
    server_src/server_protocol.h
    server_src/game/match.h
    server_src/network/matches_monitor.cpp
    server_src/network/client_monitor.h
    server_src/network/client_monitor.cpp
    server_src/game/race.h
    server_src/game/match.cpp
    server_src/server_protocol.cpp
    server_src/game/game_loop.cpp)
  add_subdirectory(server_src)
  set_project_warnings(server ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

  target_include_directories(server PRIVATE ${box2d_SOURCE_DIR})
  target_link_directories(server PRIVATE ${box2d_SOURCE_DIR})

  target_link_libraries(server PRIVATE taller_common taller_lobby box2d)
endif()

# --- EDITOR ---
if(TALLER_EDITOR)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
  add_executable(taller_editor)
  add_subdirectory(editor)
  set_project_warnings(taller_editor ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

  target_link_libraries(
    taller_editor PRIVATE taller_common SDL2pp::SDL2pp Qt6::Widgets
                          Qt6::Multimedia Qt6::Network Qt6::Core)
  target_include_directories(taller_editor PRIVATE ${libSDL2pp_SOURCE_DIR})
endif()

# =======================================================
# Testing section
# =======================================================
# =======================================================
# Target del Test de Colisión y Movimiento
# =======================================================
add_executable(collision_test tests/test_collision_movimiento.cpp
                              client_src/game/collision_manager.cpp)

target_include_directories(
  collision_test
  PRIVATE ${CMAKE_SOURCE_DIR}
          # ${libSDL2pp_SOURCE_DIR} # Necesario para SDL2pp
          ${libSDL2pp_SOURCE_DIR}/include)

target_link_libraries(
  collision_test
  PRIVATE taller_common # Para dependencias comunes (socket, etc., aunque este
                        # test no las usa)
          SDL2pp::SDL2pp # Para la ventana y renderizado
          SDL2_image # Para cargar los PNG
)

set_target_properties(collision_test PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                ${CMAKE_SOURCE_DIR})
if(TALLER_TESTS)
  add_executable(taller_tests)
  add_subdirectory(tests)
  set_project_warnings(taller_tests ${TALLER_MAKE_WARNINGS_AS_ERRORS} TRUE)

  # CORRECCIÓN CRÍTICA: Añadir TODAS las implementaciones de la lógica.
  target_sources(
    taller_tests
    PRIVATE # Implementaciones de Monitor y Partida (que contienen los
            # constructores y destructores)
            server_src/network/matches_monitor.cpp
            server_src/network/client_monitor.cpp
            server_src/game/match.cpp
            server_src/game/game_loop.cpp
            server_src/game/car.cpp
            server_src/server_protocol.cpp
            server_src/lobby/lobby_manager.cpp
            server_src/lobby/game_room.cpp
            client_src/lobby/model/lobby_client.cpp)

  # Añadir las rutas de los headers de GoogleTest/Mock
  target_include_directories(
    taller_tests
    PRIVATE ${googletest_SOURCE_DIR}/googletest/include
            ${googletest_SOURCE_DIR}/googlemock/include
            ${CMAKE_SOURCE_DIR}/server_src/
            ${CMAKE_SOURCE_DIR}/server_src/game/
            ${CMAKE_SOURCE_DIR}/server_src/network/
            ${CMAKE_SOURCE_DIR}/server_src/lobby/)

  # Link the dependencies
  target_link_libraries(
    taller_tests
    PRIVATE taller_common
            taller_lobby
            GTest::gtest_main
            Qt6::Widgets
            Qt6::Multimedia
            Qt6::Network
            Qt6::Core)
endif()
