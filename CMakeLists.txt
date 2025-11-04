cmake_minimum_required(VERSION 3.24)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS ON)

project(taller_tp
        VERSION 1.0
        HOMEPAGE_URL "https://github.com/URL-HERE"
        LANGUAGES CXX
)

# Opciones del proyecto
option(TALLER_TESTS "Enable / disable tests." ON)
option(TALLER_CLIENT "Enable / disable client program." ON)
option(TALLER_SERVER "Enable / disable server program." ON)
option(TALLER_EDITOR "Enable / disable editor program." ON)
option(TALLER_MAKE_WARNINGS_AS_ERRORS "Enable / disable warnings as errors." ON)

message(CMAKE_CXX_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")

# 1. ENCONTRAR E IMPORTAR QT
find_package(Qt6 COMPONENTS Widgets Multimedia REQUIRED)

# 2. HABILITAR EL COMPILADOR META-OBJETO (MOC)
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
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_Declare(
            SDL2_image
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
            GIT_TAG release-2.8.2
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_Declare(
            SDL2_mixer
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
            GIT_TAG release-2.8.0
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
    FetchContent_Declare(
            SDL2_ttf
            GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
            GIT_TAG release-2.22.0
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            OVERRIDE_FIND_PACKAGE
    )
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
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(box2d)

# --- yaml-cpp Integration (Parser YAML) ---
FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG master
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(yaml-cpp)

# --- GoogleTest (para tests) ---
if(TALLER_TESTS)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
endif()


# =======================================================
# Librería Común (taller_common)
# =======================================================

add_library(taller_common STATIC
        common_src/config.h
        common_src/config.cpp)
add_subdirectory(common_src)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(taller_common ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)
target_include_directories(taller_common PUBLIC .)

# Box2D está fuera para evitar propagación al cliente.
target_link_libraries(taller_common PUBLIC yaml-cpp)

# =======================================================
# Librería de Lobby (compartida entre server y tests)
# =======================================================

add_library(taller_lobby STATIC
    server_src/lobby/lobby_manager.cpp
    server_src/lobby/game_room.cpp
    server_src/lobby/lobby_server.cpp
)

target_include_directories(taller_lobby PUBLIC 
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(taller_lobby PUBLIC 
    taller_common
)

set_project_warnings(taller_lobby ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

# =======================================================
#  Program section (Ejecutables) - UNA SOLA VEZ
# =======================================================

# --- CLIENTE ---
if(TALLER_CLIENT)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

    add_executable(client
            client_src/lobby_window.h
            client_src/lobby_window.cpp)
    add_subdirectory(client_src)
    set_project_warnings(client ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    target_include_directories(client PRIVATE ${libSDL2pp_SOURCE_DIR})

    target_link_libraries(client
            PRIVATE
            taller_common
            SDL2pp::SDL2pp
            SDL2_image
            SDL2_ttf
            SDL2_mixer
            Qt6::Widgets
            Qt6::Multimedia
    )
endif()

# --- SERVIDOR ---
if(TALLER_SERVER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_executable(server)
    add_subdirectory(server_src)
    set_project_warnings(server ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    target_include_directories(server PRIVATE ${box2d_SOURCE_DIR})
    target_link_directories(server PRIVATE ${box2d_SOURCE_DIR})

    target_link_libraries(server PRIVATE 
        taller_common 
        taller_lobby
        box2d
    )
endif()

# --- EDITOR ---
if(TALLER_EDITOR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_executable(taller_editor)
    add_subdirectory(editor)
    set_project_warnings(taller_editor ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

    target_link_libraries(taller_editor
            PRIVATE
            taller_common
            SDL2pp::SDL2pp
            Qt6::Widgets
            Qt6::Multimedia)
    target_include_directories(taller_editor PRIVATE ${libSDL2pp_SOURCE_DIR})
endif()

# =======================================================
# Testing section
# =======================================================

if(TALLER_TESTS)
    add_executable(taller_tests)
    add_subdirectory(tests)
    set_project_warnings(taller_tests ${TALLER_MAKE_WARNINGS_AS_ERRORS} TRUE)

    target_include_directories(taller_tests
            PRIVATE
            ${googletest_SOURCE_DIR}/googletest/include
            ${googletest_SOURCE_DIR}/googlemock/include
            ${CMAKE_SOURCE_DIR}
    )

    target_link_libraries(taller_tests
            PRIVATE
            taller_common
            taller_lobby
            GTest::gtest_main
    )
endif()