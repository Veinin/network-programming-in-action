SET(LIBUNP_SRC readn.c writen.c readline.c wrapsock.c wrapunix.c)
ADD_LIBRARY(unp STATIC ${LIBUNP_SRC})

INSTALL(FILES unp.h DESTINATION include)