set(CAPTURE_SOURCES
    camerathread.cpp
    rtspthread.cpp
)

set(CAPTURE_HEADERS
    camerathread.h
    capturethread.h
    rtspthread.h
)

add_library(capture STATIC
    ${CAPTURE_SOURCES}
    ${CAPTURE_HEADERS}
)

target_include_directories(capture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
