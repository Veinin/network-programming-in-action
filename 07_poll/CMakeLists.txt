ADD_EXECUTABLE(poll_server poll_server.c)
ADD_EXECUTABLE(poll_client poll_client.c)

TARGET_LINK_LIBRARIES(poll_server unp)
TARGET_LINK_LIBRARIES(poll_client unp)