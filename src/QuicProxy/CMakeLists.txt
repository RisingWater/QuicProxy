SET(TARGET QuicProxy)

SET(WINPR_PATH    ${CMAKE_SOURCE_DIR}/3rdparty/winpr3)
SET(WINPR_INCLUDE ${WINPR_PATH}/include)

LIST(APPEND INCLUDE_DIR
    ./
    ../QuicCommLib
    ${CMAKE_SOURCE_DIR}/include/BaseObject
    ${CMAKE_SOURCE_DIR}/include/EnhanceLib
    ${CMAKE_SOURCE_DIR}/3rdparty/msquic/include
    ${WINPR_INCLUDE}
    )

INCLUDE_DIRECTORIES(${INCLUDE_DIR}) 

SET(CMAKE_C_FLAGS "-fPIC")
SET(CMAKE_CXX_FLAGS "-fPIC")

LIST(APPEND SRC_FILES
    ProxyProcessor.cpp
    ProxyService.cpp
    ClientSession.cpp
    SocketProxy.cpp
    main.cpp
    )

ADD_EXECUTABLE(${TARGET} ${SRC_FILES})

TARGET_LINK_LIBRARIES(${TARGET}
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libQuicCommLib.a
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libEnhanceLib.a
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libcorelib.a
    ${WINPR_PATH}/lib/${LIB_PLATFORM}/libwinpr3.a
    ${CMAKE_SOURCE_DIR}/3rdparty/msquic/lib/${LIB_PLATFORM}/libmsquic.so.2
    uuid
    ssl
    pthread
    )
