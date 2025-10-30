target_sources(taller_common
    PRIVATE
    # .cpp files
    socket.cpp
    resolver.cpp
    liberror.cpp
    resolvererror.cpp
    
    PUBLIC
    # .h files
    socket.h
    thread.h
    queue.h
    resolver.h
    resolvererror.h
    liberror.h
)