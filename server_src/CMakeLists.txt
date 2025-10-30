target_sources(server
    PRIVATE
    # .cpp files
    server_main.cpp
    server.cpp
    server_acceptor.cpp
    server_client_handler.cpp
    server_monitor.cpp
    server_receiver.cpp
    server_sender.cpp
    server_protocol.cpp
    server_game.cpp
    server_car.cpp
    
    PUBLIC
    # .h files
    server.h
    server_acceptor.h
    server_client_handler.h
    server_monitor.h
    server_receiver.h
    server_sender.h
    server_protocol.h
    server_game.h
    server_car.h
)