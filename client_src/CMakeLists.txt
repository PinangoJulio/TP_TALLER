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
    
)

add_executable(lobby_client_test
    lobby/model/lobby_client.cpp      # Lógica del cliente
    lobby/model/lobby_client.h        # Header
    ${CMAKE_SOURCE_DIR}/tests/lobby_integration_test.cpp  
)

target_include_directories(lobby_client_test
    PRIVATE
    ${CMAKE_SOURCE_DIR}  
)

target_link_libraries(lobby_client_test
    PRIVATE
    taller_common
)

set_target_properties(lobby_client_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)