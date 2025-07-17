set(UI_SOURCES
    AudioDetectionPage.cpp
    VideoDetectionPage.cpp
    MonitorListPage.cpp
    DetectionListPage.cpp
    ShowMonitorPage.cpp
)

set(UI_HEADERS
    AudioDetectionPage.h
    VideoDetectionPage.h
    MonitorListPage.h
    DetectionListPage.h
    ShowMonitorPage.h
)

add_library(ui STATIC
    ${UI_SOURCES}
    ${UI_HEADERS}
)

# 添加更完整的包含路径
target_include_directories(ui
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}  # 添加 Network
    ${Qt5MultimediaWidgets_INCLUDE_DIRS}  # 添加 MultimediaWidgets

    ../widgets
    ../core
    ../ai
)

# 添加更完整的链接库
target_link_libraries(ui PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network  # 添加 Network
    Qt5::MultimediaWidgets  # 添加 MultimediaWidgets
    widgets
    core
    ai
)
