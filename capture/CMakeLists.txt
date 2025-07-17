set(CAPTURE_SOURCES
    camerathread.cpp
    rtspthread.cpp
    usbcapturethread.cpp  # 确保包含新文件
    recordmanager.cpp
)

set(CAPTURE_HEADERS
    camerathread.h
    capturethread.h
    rtspthread.h
    usbcapturethread.h  # 确保包含新头文件
    recordmanager.h
)

add_library(capture STATIC
    ${CAPTURE_SOURCES}
    ${CAPTURE_HEADERS}

)

target_include_directories(capture PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    # 添加 OpenCV 包含路径
    ${OpenCV_INCLUDE_DIRS}
)

# 添加 OpenCV 库路径
target_link_directories(capture PUBLIC ${OpenCV_LIB_DIR})

# 添加 OpenCV 依赖
target_link_libraries(capture PUBLIC ${OpenCV_LIBS})
