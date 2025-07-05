set(MAIN_SOURCES
    main.cpp
    secureVision.cpp
)

set(MAIN_HEADERS
    secureVision.h
)

add_library(main STATIC
    ${MAIN_SOURCES}
    ${MAIN_HEADERS}
)

# 包含路径设置
target_include_directories(main
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}  # 当前模块头文件
    ${Qt5Widgets_INCLUDE_DIRS}   # Qt Widgets头文件路径
    ${Qt5Core_INCLUDE_DIRS}      # Qt Core头文件路径
    ${Qt5Gui_INCLUDE_DIRS}       # Qt Gui头文件路径
    ${Qt5Multimedia_INCLUDE_DIRS} # Qt Multimedia头文件路径
    ${Qt5Network_INCLUDE_DIRS}   # Qt Network头文件路径

    # 添加ui和capture模块的包含路径
    ../ui
    ../capture
)

# 链接库设置
target_link_libraries(main PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network

    # 添加对ui和capture模块的依赖
    ui
    capture
)
