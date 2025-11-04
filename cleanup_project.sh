#!/bin/bash
set -e

echo "🧹 Limpiando estructura del proyecto..."

# ============================================
# 1. ELIMINAR archivos innecesarios
# ============================================
echo "❌ Eliminando archivos de ejemplo y duplicados..."

rm -f common_src/foo.h common_src/foo.cpp
rm -f common_src/utils.h
rm -f common_src/protocol.h common_src/protocol.cpp
rm -f common_src/common_socket.h common_src/common_socket.cpp

rm -f server_src/server_protocol.h server_src/server_protocol.cpp
rm -f server_src/server_main.cpp

rm -f client_src/client.h client_src/client.cpp
rm -f client_src/client_protocol.h client_src/client_protocol.cpp
rm -f client_src/lobby_protocol.h

rm -f tests/foo.cpp

# ============================================
# 2. CREAR directorios de la nueva estructura
# ============================================
echo "📁 Creando nuevos directorios..."

mkdir -p server_src/game
mkdir -p server_src/game/physics
mkdir -p server_src/network
mkdir -p client_src/lobby
mkdir -p client_src/game
mkdir -p client_src/game/renderer
mkdir -p tests

# ============================================
# 3. MOVER archivos del servidor
# ============================================
echo "📦 Reorganizando archivos del servidor..."

# Game
if [ -f server_src/server_game.h ]; then
    mv server_src/server_game.h server_src/game/game_loop.h
    mv server_src/server_game.cpp server_src/game/game_loop.cpp
fi

if [ -f server_src/server_car.h ]; then
    mv server_src/server_car.h server_src/game/car.h
    mv server_src/server_car.cpp server_src/game/car.cpp
fi

# Network
if [ -f server_src/server_client_handler.h ]; then
    mv server_src/server_client_handler.h server_src/network/client_handler.h
    mv server_src/server_client_handler.cpp server_src/network/client_handler.cpp
fi

if [ -f server_src/server_receiver.h ]; then
    mv server_src/server_receiver.h server_src/network/receiver.h
    mv server_src/server_receiver.cpp server_src/network/receiver.cpp
fi

if [ -f server_src/server_sender.h ]; then
    mv server_src/server_sender.h server_src/network/sender.h
    mv server_src/server_sender.cpp server_src/network/sender.cpp
fi

if [ -f server_src/server_monitor.h ]; then
    mv server_src/server_monitor.h server_src/network/monitor.h
    mv server_src/server_monitor.cpp server_src/network/monitor.cpp
fi

# Acceptor
if [ -f server_src/server_acceptor.h ]; then
    mv server_src/server_acceptor.h server_src/acceptor.h
    mv server_src/server_acceptor.cpp server_src/acceptor.cpp
fi

# ============================================
# 4. MOVER archivos del cliente
# ============================================
echo "📦 Reorganizando archivos del cliente..."

if [ -f client_src/lobby_window.h ]; then
    mv client_src/lobby_window.h client_src/lobby/
    mv client_src/lobby_window.cpp client_src/lobby/
fi

if [ -f client_src/lobby_client.h ]; then
    mv client_src/lobby_client.h client_src/lobby/
    mv client_src/lobby_client.cpp client_src/lobby/
fi

# Test client → tests
if [ -f client_src/lobby_test_client.cpp ]; then
    mv client_src/lobby_test_client.cpp tests/lobby_integration_test.cpp
fi

# ============================================
# 5. ACTUALIZAR includes en archivos movidos
# ============================================
echo "🔧 Actualizando rutas de includes..."

# Actualizar includes en game_loop
if [ -f server_src/game/game_loop.h ]; then
    sed -i 's|server_car.h|car.h|g' server_src/game/game_loop.h
    sed -i 's|server_monitor.h|../network/monitor.h|g' server_src/game/game_loop.h
fi

if [ -f server_src/game/game_loop.cpp ]; then
    sed -i 's|server_game.h|game_loop.h|g' server_src/game/game_loop.cpp
fi

# Actualizar includes en network
for file in server_src/network/*.h server_src/network/*.cpp; do
    if [ -f "$file" ]; then
        sed -i 's|server_client_handler|client_handler|g' "$file"
        sed -i 's|server_receiver|receiver|g' "$file"
        sed -i 's|server_sender|sender|g' "$file"
        sed -i 's|server_monitor|monitor|g' "$file"
        sed -i 's|server_protocol|../game/game_protocol|g' "$file"
    fi
done

# Actualizar includes en acceptor
if [ -f server_src/acceptor.h ]; then
    sed -i 's|server_client_handler.h|network/client_handler.h|g' server_src/acceptor.h
    sed -i 's|server_monitor.h|network/monitor.h|g' server_src/acceptor.h
fi

if [ -f server_src/acceptor.cpp ]; then
    sed -i 's|server_acceptor.h|acceptor.h|g' server_src/acceptor.cpp
fi

echo "✅ Limpieza completada!"
echo ""
echo "📋 Próximos pasos:"
echo "1. Crear common_src/dtos.h (fusionar utils.h)"
echo "2. Crear common_src/game_protocol.h/cpp"
echo "3. Actualizar server_src/CMakeLists.txt"
echo "4. Compilar: make debug"