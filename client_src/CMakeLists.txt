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
    game/collision_manager.cpp

    #ranking
    ranking/final_ranking.cpp
    
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
    game/collision_manager.h
    
    #ranking
    ranking/final_ranking.h
)

