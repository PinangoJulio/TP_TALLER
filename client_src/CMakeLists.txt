target_sources(client
    PRIVATE
    # .cpp files
    client_main.cpp
    client.cpp
    client_protocol.cpp
    
    PUBLIC
    # .h files
    client.h
    client_protocol.h
)