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

target_include_directories(main
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}

    ../ui
    ../capture
)

target_link_libraries(main PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::Network

    ui
    capture
    ai
)
