cmake_minimum_required(VERSION 4.0)
project(threads_2025c2_loudemeglio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(common_src)

set(COMMON_SOURCES
        common_src/liberror.cpp
        common_src/queue.h
        common_src/resolver.cpp
        common_src/resolver.h
        common_src/resolvererror.cpp
        common_src/resolvererror.h
        common_src/socket.cpp
        common_src/socket.h
        common_src/thread.h
        common_src/utils.h)

# CLiente
add_executable(client
        client_src/client_main.cpp
         ${COMMON_SOURCES}
        client_src/client_protocol.cpp
        common_src/utils.h
        client_src/client.h
        client_src/client.cpp
        client_src/client_protocol.h
)

#Servidor
add_executable(server
    server_src/server_main.cpp
    ${COMMON_SOURCES}
        server_src/server.h
        server_src/server.cpp
        server_src/server_protocol.cpp
        server_src/server_game.cpp
        server_src/server_acceptor.h
        server_src/server_monitor.h
        server_src/server_acceptor.cpp
        server_src/server_client_handler.h
        server_src/server_client_handler.cpp
        server_src/server_receiver.h
        server_src/server_receiver.cpp
        server_src/server_sender.h
        server_src/server_sender.cpp
        server_src/server_monitor.cpp
        server_src/server_car.h
        server_src/server_car.cpp
        server_src/server_car.cpp
)
