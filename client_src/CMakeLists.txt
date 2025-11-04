target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    client.cpp
    client_protocol.cpp
    lobby_window.cpp
    match_selection_window.cpp
    garage_window.cpp
    waiting_room_window.cpp
    
    PUBLIC
    # .h files
    client.h
    client_protocol.h
    lobby_window.h
    match_selection_window.h
    garage_window.h
    waiting_room_window.h
)