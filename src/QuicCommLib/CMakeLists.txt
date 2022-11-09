SET(TARGET QuicCommLib)

SET(WINPR_PATH    ${CMAKE_SOURCE_DIR}/3rdparty/winpr3)
SET(WINPR_INCLUDE ${WINPR_PATH}/include)

LIST(APPEND INCLUDE_DIR
    ${CMAKE_SOURCE_DIR}/include/BaseObject
    ${CMAKE_SOURCE_DIR}/include/EnhanceLib
    ${CMAKE_SOURCE_DIR}/include/corelib
    ${CMAKE_SOURCE_DIR}/3rdparty/msquic/include
    ${WINPR_INCLUDE}
    )

INCLUDE_DIRECTORIES(${INCLUDE_DIR}) 

SET(CMAKE_C_FLAGS "-fPIC")
SET(CMAKE_CXX_FLAGS "-fPIC")

LIST(APPEND SRC_FILES
    QUICBase.cpp
    QUICServer.cpp
    QUICClient.cpp
    )


ADD_LIBRARY(${TARGET} STATIC ${SRC_FILES})
