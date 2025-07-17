set(WIDGET_SOURCES
    AudioUploader.cpp
    CardWidget.cpp
    MonitorWidget.cpp
    QcustomPlot.cpp
    RecordingDialog.cpp
    WaveFormManager.cpp
    aicontrolwidget.cpp
)

add_library(widgets STATIC
    ${WIDGET_SOURCES}
    AudioUploader.h
    CardWidget.h
    MonitorWidget.h
    QcustomPlot.h
    RecordingDialog.h
    WaveFormManager.h
    aicontrolwidget.h
)

target_include_directories(widgets
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
)

target_link_libraries(widgets PUBLIC
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    Qt5::Network
)
