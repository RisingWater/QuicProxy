SET(TARGET LogEx)

SET(WINPR_PATH    ${CMAKE_SOURCE_DIR}/3rdparty/winpr3)
SET(WINPR_INCLUDE ${WINPR_PATH}/include)

LIST(APPEND INCLUDE_DIR
    ./
    ${CMAKE_SOURCE_DIR}/include/BaseObject
    ${WINPR_INCLUDE}
    )

INCLUDE_DIRECTORIES(${INCLUDE_DIR}) 

SET(CMAKE_C_FLAGS "-fPIC")
SET(CMAKE_CXX_FLAGS "-fPIC")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

LIST(APPEND SRC_FILES
    log.cpp
    LogEX.cpp
    )


ADD_LIBRARY(${TARGET} SHARED ${SRC_FILES})
