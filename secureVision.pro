# secureVision.pro

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += multimedia widgets multimediawidgets printsupport

TARGET = SecureVision
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++14

# 包含 pages.pri 和 components.pri
include(pages.pri)
include(components.pri)
include(manager.pri)

# 主页面和其他源文件
SOURCES += \
    src/main.cpp \
    src/secureVision.cpp

HEADERS += \
    include/secureVision.h

RESOURCES += \
    secureVision.qrc

# 添加RKMedia相关头文件路径
INCLUDEPATH += /home/zhenglongbin/RV1126/external/rkmedia/include
INCLUDEPATH += /home/zhenglongbin/RV1126/app/alientek/atk_rkmedia/library/include

# 添加库文件的路径
LIBS += -L/home/zhenglongbin/RV1126/buildroot/output/alientek_rv1126/host/arm-buildroot-linux-gnueabihf/sysroot/oem/usr/lib

# 链接需要的库文件
LIBS += -leasymedia -lthird_media -lrockchip_mpp
LIBS += -latk_camera -lsample_common_isp

# 设置运行时库搜索路径（指向开发板上的实际路径）
QMAKE_LFLAGS += -Wl,-rpath,/oem/usr/lib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
