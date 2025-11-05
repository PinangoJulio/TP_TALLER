target_sources(client
    PRIVATE
    # .cpp files
    main.cpp
    lobby/lobby_window.cpp
    lobby/lobby_client.cpp  
    
    PUBLIC
    # .h files
    lobby/lobby_window.h
    lobby/lobby_client.h
)

# ❌ ELIMINAR TODO ESTO (ya no es necesario):
# add_executable(lobby_client_test
#     lobby/lobby_client.cpp
# )
# 
# target_link_libraries(lobby_client_test
#     PRIVATE
#     taller_common
# )
# 
# set_target_properties(lobby_client_test PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
# )