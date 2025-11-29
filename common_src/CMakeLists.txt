target_sources(taller_common
    PRIVATE
    # .cpp files
    socket.cpp
    resolver.cpp
    liberror.cpp
    resolvererror.cpp
    config.cpp
    lobby_protocol.cpp
    game_state.cpp
    collision_manager.cpp

    PUBLIC
    # .h files
    socket.h
    thread.h
    queue.h
    resolver.h
    resolvererror.h
    liberror.h
    dtos.h
    config.h
    lobby_protocol.h
    game_protocol.h
    game_state.h
    collision_manager.h
    #common_types.h
)