target_sources(taller_common
    PRIVATE
    # .cpp files
    foo.cpp
    socket.cpp
    resolver.cpp
    liberror.cpp
    resolvererror.cpp
    
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
)