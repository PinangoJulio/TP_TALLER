target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    lobby/lobby_client.cpp  

    # Front
    lobby/front/lobby_window.cpp
    lobby/front/name_input_window.cpp
    lobby/front/match_selection_window.cpp
    lobby/front/create_match_window.cpp
    lobby/front/garage_window.cpp
    lobby/front/waiting_room_window.cpp
    
    PUBLIC
    # .h files
    lobby/lobby_client.h

    #Front
    lobby/front/lobby_window.h
    lobby/front/common_types.h
    lobby/front/name_input_window.h
    lobby/front/match_selection_window.h
    lobby/front/create_match_window.h
    lobby/front/garage_window.h
    lobby/front/waiting_room_window.h

)

add_executable(lobby_client_test
    lobby/lobby_client.cpp      # Lógica del cliente
    lobby/lobby_client.h        # Header
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