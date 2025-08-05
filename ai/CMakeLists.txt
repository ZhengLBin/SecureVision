cmake_minimum_required(VERSION 3.5)

project(ai LANGUAGES CXX)

set(AI_SOURCES
    aidetectionthread.cpp
    motiondetector.cpp
    detectionvisualizer.cpp
    facerecognitionmanager.cpp
    facedatabase.cpp

)

set(AI_HEADERS
    aidetectionthread.h
    motiondetector.h
    aitypes.h
    detectionvisualizer.h
    facerecognitionmanager.h
    facedatabase.h
)

# 定义 ai 库
add_library(ai STATIC ${AI_SOURCES} ${AI_HEADERS})

# 启用 Qt 的自动功能（MOC / UIC / RCC）
set_target_properties(ai PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# 添加包含目录
target_include_directories(ai PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${OpenCV_INCLUDE_DIRS}
)

# 链接 Qt5 和 OpenCV
target_link_libraries(ai
    Qt5::Core
    ${OpenCV_LIBS}
    Qt5::Sql
)
