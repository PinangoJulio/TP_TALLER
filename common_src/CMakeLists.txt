target_sources(taller_common
    PRIVATE
    # .cpp files
    foo.cpp
    socket.cpp
    resolver.cpp
    liberror.cpp
    resolvererror.cpp
    config.cpp
    lobby_protocol.cpp 
    
    PUBLIC
    # .h files
    foo.h
    socket.h
    thread.h
    queue.h
    resolver.h
    resolvererror.h
    liberror.h
    utils.h
    config.h
    lobby_protocol.h  
)