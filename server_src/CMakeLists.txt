target_sources(server
    PRIVATE
    # Main
    main.cpp
    server.cpp
    acceptor.cpp
    
    # Lobby
    lobby/lobby_server.cpp
    lobby/lobby_manager.cpp
    lobby/game_room.cpp
    
    # Game
    game/game_loop.cpp  
    game/car.cpp
    game/match.cpp 
    game/race.cpp
    game/collision_handler.cpp 
    game/obstacle.cpp
    game/map_loader.cpp
    game/checkpoint_manager.cpp      
    
    # Network
    network/client_handler.cpp
    network/receiver.cpp
    network/sender.cpp
    network/monitor.cpp
    
    PUBLIC
    # Headers
    server.h
    acceptor.h
    lobby/lobby_server.h
    lobby/lobby_manager.h
    lobby/game_room.h
    game/game_loop.h
    game/car.h
    game/match.h   
    game/race.h    
    game/player.h
    game/collision_handler.h
    game/obstacle.h
    game/map_loader.h
    game/checkpoint_manager.h  
    network/client_handler.h
    network/receiver.h
    network/sender.h
    network/monitor.h
)