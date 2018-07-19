ADD_EXECUTABLE(select_server select_server.c)
ADD_EXECUTABLE(select_client select_client.c)

TARGET_LINK_LIBRARIES(select_server unp)
TARGET_LINK_LIBRARIES(select_client unp)