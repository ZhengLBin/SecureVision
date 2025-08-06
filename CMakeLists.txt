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

# 7. RockX 和 RKNN 库配置（新增）
set(ROCKX_INCLUDE_DIR ${SYSROOT_PATH}/usr/include)
set(ROCKX_LIB_DIR ${SYSROOT_PATH}/usr/lib)

# 查找 RockX 相关库
find_library(ROCKX_LIB
    NAMES rockx librockx
    PATHS ${ROCKX_LIB_DIR}
    PATH_SUFFIXES arm-linux-gnueabihf
    NO_DEFAULT_PATH
)

find_library(RKNN_API_LIB
    NAMES rknn_api librknn_api rknnrt librknnrt
    PATHS ${ROCKX_LIB_DIR}
    PATH_SUFFIXES arm-linux-gnueabihf
    NO_DEFAULT_PATH
)

# 查找 SQLite3 库
find_library(SQLITE3_LIB
    NAMES sqlite3 libsqlite3
    PATHS ${ROCKX_LIB_DIR}
    PATH_SUFFIXES arm-linux-gnueabihf
    NO_DEFAULT_PATH
)

# 验证库文件
if(NOT ROCKX_LIB)
    message(WARNING "RockX library not found in ${ROCKX_LIB_DIR}")
    # 列出可用的库文件进行调试
    file(GLOB AVAILABLE_LIBS "${ROCKX_LIB_DIR}/*rockx*" "${ROCKX_LIB_DIR}/*/*rockx*")
    message(STATUS "Available RockX related files: ${AVAILABLE_LIBS}")
endif()

if(NOT RKNN_API_LIB)
    message(WARNING "RKNN API library not found in ${ROCKX_LIB_DIR}")
    file(GLOB AVAILABLE_RKNN_LIBS "${ROCKX_LIB_DIR}/*rknn*" "${ROCKX_LIB_DIR}/*/*rknn*")
    message(STATUS "Available RKNN related files: ${AVAILABLE_RKNN_LIBS}")
endif()

if(NOT SQLITE3_LIB)
    message(WARNING "SQLite3 library not found in ${ROCKX_LIB_DIR}")
    file(GLOB AVAILABLE_SQLITE_LIBS "${ROCKX_LIB_DIR}/*sqlite*" "${ROCKX_LIB_DIR}/*/*sqlite*")
    message(STATUS "Available SQLite related files: ${AVAILABLE_SQLITE_LIBS}")
endif()

# 7. 查找 Qt5 库（必须在设置 SYSROOT 后）
set(Qt5_DIR ${SYSROOT_PATH}/usr/lib/cmake/Qt5)  # 根据实际路径调整
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia Network MultimediaWidgets PrintSupport Sql)

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

    #新增 RockX 相关路径
    ${ROCKX_INCLUDE_DIR}
    ${ROCKX_INCLUDE_DIR}/rockx
    ${ROCKX_INCLUDE_DIR}/rknn
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
    Qt5::Sql

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
    ${ROCKX_LIB}
    ${RKNN_API_LIB}
    ${SQLITE3_LIB}
)

# 13. 安装设置
set_target_properties(SecureVision PROPERTIES
    INSTALL_RPATH "/oem/usr/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# 14. 添加安装规则（移除重复）
install(TARGETS SecureVision
    RUNTIME DESTINATION bin
)

# 🆕 15. 添加RockX模型文件安装规则（仅安装必需的3个模型）
set(ROCKX_MODEL_SOURCE_DIR "${CMAKE_SOURCE_DIR}/data/models/rockx_data")

# 定义人脸识别必需的模型文件
set(REQUIRED_FACE_MODELS
    "face_detection_v3_fast.data"    # 人脸检测
    "face_landmark5.data"            # 5点关键点检测
    "face_recognition.data"          # 人脸识别特征提取
)

# 检查模型文件目录是否存在
if(EXISTS ${ROCKX_MODEL_SOURCE_DIR})
    message(STATUS "Found RockX models directory: ${ROCKX_MODEL_SOURCE_DIR}")

    # 列出源目录中的所有文件（调试用）
    file(GLOB ALL_MODEL_FILES "${ROCKX_MODEL_SOURCE_DIR}/*.data")
    message(STATUS "All available model files: ${ALL_MODEL_FILES}")

    # 检查并安装每个必需的模型文件
    set(MODELS_TO_INSTALL "")
    foreach(MODEL_FILE ${REQUIRED_FACE_MODELS})
        set(FULL_MODEL_PATH "${ROCKX_MODEL_SOURCE_DIR}/${MODEL_FILE}")
        if(EXISTS ${FULL_MODEL_PATH})
            message(STATUS "✓ Will install required model: ${MODEL_FILE}")
            list(APPEND MODELS_TO_INSTALL ${FULL_MODEL_PATH})
        else()
            message(WARNING "✗ Missing required model: ${MODEL_FILE}")
            message(WARNING "  Expected at: ${FULL_MODEL_PATH}")
        endif()
    endforeach()

    # 仅安装找到的必需模型文件
    if(MODELS_TO_INSTALL)
        # 只安装到一个位置，避免路径重复
        install(FILES ${MODELS_TO_INSTALL}
            DESTINATION share/rockx_data
            PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
        )

        list(LENGTH MODELS_TO_INSTALL MODELS_COUNT)
        message(STATUS "Will install ${MODELS_COUNT} model files to: /share/rockx_data")
    else()
        message(FATAL_ERROR "No required model files found!")
    endif()

else()
    message(FATAL_ERROR "RockX models directory not found: ${ROCKX_MODEL_SOURCE_DIR}")
endif()

# # 🆕 16. 创建启动脚本
set(LAUNCH_SCRIPT_CONTENT "#!/bin/bash
# SecureVision Launch Script
export ROCKX_MODEL_PATH=/usr/local/share/rockx_data
export QT_QPA_PLATFORM=linuxfb
export QT_QPA_FONTDIR=/usr/share/fonts

# 检查模型文件
if [ ! -d \"/usr/local/share/rockx_data\" ]; then
    echo \"Warning: RockX model files not found at /usr/local/share/rockx_data\"
    echo \"Looking for alternative locations...\"

    # 尝试其他可能的位置
    if [ -d \"/share/rockx_data\" ]; then
        export ROCKX_MODEL_PATH=/share/rockx_data
        echo \"Using models from /share/rockx_data\"
    else
        echo \"Error: No RockX model files found!\"
        exit 1
    fi
fi

# 创建运行时数据目录（如果不存在）
mkdir -p \${HOME}/.local/share/SecureVision/database
mkdir -p \${HOME}/.local/share/SecureVision/faces

echo \"Starting SecureVision with model path: \$ROCKX_MODEL_PATH\"
exec /usr/local/bin/SecureVision \"\$@\"
")

# 创建启动脚本文件
file(WRITE "${CMAKE_BINARY_DIR}/securevision.sh" "${LAUNCH_SCRIPT_CONTENT}")

# 安装启动脚本
install(FILES "${CMAKE_BINARY_DIR}/securevision.sh"
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    RENAME securevision
)

