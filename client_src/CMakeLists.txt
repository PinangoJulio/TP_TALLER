target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    lobby/model/lobby_client.cpp  

    # view
    lobby/view/base_lobby.cpp
    lobby/view/lobby_window.cpp
    lobby/view/name_input_window.cpp
    lobby/view/match_selection_window.cpp
    lobby/view/create_match_window.cpp
    lobby/view/garage_window.cpp
    lobby/view/waiting_room_window.cpp

    # controller
    lobby/controller/lobby_controller.cpp

    #game
    #game/collision_manager.cpp
    game/game_renderer.cpp
    client_event_handler.cpp
    
    #threads/protocol
    client_receiver.cpp
    client_sender.cpp

    PUBLIC
    # .h files
    lobby/model/lobby_client.h
    
    #view
    lobby/view/lobby_window.h
    lobby/view/base_lobby.h
    lobby/view/common_types.h
    lobby/view/name_input_window.h
    lobby/view/match_selection_window.h
    lobby/view/create_match_window.h
    lobby/view/garage_window.h
    lobby/view/waiting_room_window.h
    
    # controller
    lobby/controller/lobby_controller.h
    
    #game
    game/game_renderer.h
    #game/collision_manager.h
    client_event_handler.h
    
    #threads/protocol
    client_receiver.h
    client_sender.h
    
)
