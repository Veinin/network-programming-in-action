ADD_EXECUTABLE(echo_server echo_server.c) 
ADD_EXECUTABLE(echo_client echo_client.c) 

TARGET_LINK_LIBRARIES(echo_server unp)
TARGET_LINK_LIBRARIES(echo_client unp)