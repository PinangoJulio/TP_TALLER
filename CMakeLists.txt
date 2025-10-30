cmake_minimum_required(VERSION 3.24)

if (POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS ON)

project(tpfinal_need_for_speed
    VERSION 1.0
    HOMEPAGE_URL "https://github.com/TU-REPO-AQUI"
    LANGUAGES CXX
)

# Opciones del proyecto
option(TALLER_TESTS "Enable / disable tests." ON)
option(TALLER_CLIENT "Enable / disable client program." ON)
option(TALLER_SERVER "Enable / disable server program." ON)
option(TALLER_EDITOR "Enable / disable editor program." ON)
option(TALLER_MAKE_WARNINGS_AS_ERRORS "Enable / disable warnings as errors." ON)

message(CMAKE_CXX_COMPILER_ID="${CMAKE_CXX_COMPILER_ID}")

# ============================================================================
# Common Library (código compartido)
# ============================================================================
add_library(taller_common STATIC)

# Agregar subdirectorios con código común
add_subdirectory(common_src/)

# Configurar warnings
include(cmake/CompilerWarnings.cmake)
set_project_warnings(taller_common ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)

# Headers públicos de common
target_include_directories(taller_common PUBLIC .)

# ============================================================================
# SDL2 y SDL2pp (solo para client y editor)
# ============================================================================
if(TALLER_CLIENT OR TALLER_EDITOR)
    include(FetchContent)
    
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

# ============================================================================
# Cliente
# ============================================================================
if(TALLER_CLIENT)
    add_executable(client)
    add_dependencies(client taller_common SDL2pp::SDL2pp)
    add_subdirectory(client_src)
    set_project_warnings(client ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)
    target_include_directories(client PRIVATE ${libSDL2pp_SOURCE_DIR})
    target_link_libraries(client taller_common SDL2pp::SDL2pp)
endif()

# ============================================================================
# Servidor
# ============================================================================
if(TALLER_SERVER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_executable(server)
    add_dependencies(server taller_common)
    add_subdirectory(server_src)
    set_project_warnings(server ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)
    target_link_libraries(server taller_common)
endif()

# ============================================================================
# Editor
# ============================================================================
if(TALLER_EDITOR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_executable(taller_editor)
    add_dependencies(taller_editor taller_common SDL2pp::SDL2pp)
    add_subdirectory(editor)
    set_project_warnings(taller_editor ${TALLER_MAKE_WARNINGS_AS_ERRORS} FALSE)
    target_include_directories(taller_editor PRIVATE ${libSDL2pp_SOURCE_DIR})
    target_link_libraries(taller_editor taller_common SDL2pp::SDL2pp)
endif()

# ============================================================================
# Tests (GoogleTest)
# ============================================================================
if(TALLER_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)

    add_executable(taller_tests)
    add_dependencies(taller_tests taller_common)
    target_include_directories(taller_tests PUBLIC .)
    add_subdirectory(tests/)
    set_project_warnings(taller_tests ${TALLER_MAKE_WARNINGS_AS_ERRORS} TRUE)
    target_include_directories(taller_tests
        PUBLIC
        ${gtest_SOURCE_DIR}/include
        ${gmock_SOURCE_DIR}/include
    )
    target_link_libraries(taller_tests
        taller_common
        GTest::gtest_main
    )
endif()
