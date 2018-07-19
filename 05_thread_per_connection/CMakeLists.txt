ADD_EXECUTABLE(tpc_server tpc_server.c) 
TARGET_LINK_LIBRARIES(tpc_server unp pthread)