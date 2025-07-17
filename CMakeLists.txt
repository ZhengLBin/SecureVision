cmake_minimum_required(VERSION 3.5)
project(SecureVision LANGUAGES CXX)

# 1. 交叉编译工具链设置（必须最先设置）
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 工具链路径（根据您的实际路径调整）
set(TOOLCHAIN_DIR /opt/atk-dlrv1126-toolchain)
set(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/usr/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/usr/bin/arm-linux-gnueabihf-g++)

# 2. 系统根目录设置（关键修正）
# 注意：这里使用 buildroot 输出的 host 目录下的 sysroot
set(SYSROOT_PATH ${TOOLCHAIN_DIR}/arm-buildroot-linux-gnueabihf/sysroot)
set(CMAKE_FIND_ROOT_PATH ${SYSROOT_PATH})
set(CMAKE_SYSROOT ${SYSROOT_PATH})

# 3. 搜索规则设置
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 4. 基础编译设置
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 5. FFmpeg 路径设置（使用您找到的实际路径）
set(FFMPEG_INCLUDE_DIRS
    ${SYSROOT_PATH}/usr/include
    /home/zhenglongbin/code/rv1126/buildroot/output/alientek_rv1126/build/ffmpeg-4.1.3
)

# 6. OpenCV 配置 - 优化版本
# 设置 OpenCV 根目录
set(OpenCV_ROOT ${CMAKE_SOURCE_DIR}/buildroot/output/alientek_rv1126/build/opencv3-3.4.12/buildroot-build)

# 设置包含路径
set(OpenCV_INCLUDE_DIRS
    ${OpenCV_ROOT}/../include
    ${OpenCV_ROOT}/../include/opencv
    ${OpenCV_ROOT}/../include/opencv2
)

# 设置库目录
set(OpenCV_LIB_DIR ${OpenCV_ROOT}/lib)

# 手动设置 OpenCV 库文件
set(OpenCV_LIBS
    opencv_core
    opencv_imgproc
    opencv_videoio
    opencv_highgui
)

# 7. 查找 Qt5 库（必须在设置 SYSROOT 后）
set(Qt5_DIR ${SYSROOT_PATH}/usr/lib/cmake/Qt5)  # 根据实际路径调整
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network MultimediaWidgets PrintSupport)

# 8. 包含目录设置
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
    ${OpenCV_INCLUDE_DIRS}
    # RKMedia 路径
    /home/zhenglongbin/code/rv1126/external/rkmedia/include
    /home/zhenglongbin/code/rv1126/app/alientek/atk_rkmedia/library/include
    ${Qt5Multimedia_INCLUDE_DIRS}
)

if(Qt5_FOUND)
    # 将Qt包含目录添加到全局包含
    include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})

    # 打印调试信息（可选）
    message(STATUS "Qt5Core include dirs: ${Qt5Core_INCLUDE_DIRS}")
    message(STATUS "Qt5Gui include dirs: ${Qt5Gui_INCLUDE_DIRS}")
endif()

# 9. 添加库文件搜索路径
link_directories(
    ${OpenCV_LIB_DIR}
    # 其他可能的库目录...
)

# 10. 添加子目录
add_subdirectory(ai)
add_subdirectory(widgets)
add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(capture)
add_subdirectory(main)

# add_subdirectory(record)

# 11. 主程序配置
set(RESOURCE_FILE Resource/secureVision.qrc)

# 生成资源文件（Qt5 方式）
qt5_add_resources(RESOURCE_FILES ${RESOURCE_FILE})

add_executable(SecureVision
    ${RESOURCE_FILES}
    # 其他源文件...
)

# 12. 链接库配置
target_link_libraries(SecureVision PRIVATE
    capture
    widgets
    core
    ai
    ui
    main

    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::PrintSupport

    avformat
    avcodec
    avutil
    swscale

    easymedia
    third_media
    rockchip_mpp
    atk_camera
    sample_common_isp
    pthread

    ${OpenCV_LIBS}  # 链接 OpenCV 库
)

# 13. 安装设置
set_target_properties(SecureVision PROPERTIES
    INSTALL_RPATH "/oem/usr/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 14. 添加安装规则
install(TARGETS SecureVision
    RUNTIME DESTINATION bin
)

# 15. 打印配置信息用于调试
message(STATUS "==============================================")
message(STATUS "OpenCV Configuration:")
message(STATUS "  OpenCV_ROOT: ${OpenCV_ROOT}")
message(STATUS "  Include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "  Library dir: ${OpenCV_LIB_DIR}")
message(STATUS "  Libraries: ${OpenCV_LIBS}")
message(STATUS "==============================================")
