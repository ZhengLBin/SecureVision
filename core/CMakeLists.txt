set(CORE_SOURCES
    Device.cpp
    DeviceManager.cpp
)

set(CORE_HEADERS
    Device.h
    DeviceManager.h
)

add_library(core STATIC
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
