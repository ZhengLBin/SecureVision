# components.pri
INCLUDEPATH += $$PWD
SOURCES += \
    $$PWD/src/components/RecordingDialog.cpp \
    $$PWD/src/components/CardWidget.cpp \
    $$PWD/src/components/WaveFormManager.cpp \
    $$PWD/src/components/AudioUploader.cpp \
    $$PWD/src/components/QcustomPlot.cpp \
    $$PWD/src/components/MonitorWidget.cpp \
    $$PWD/src/components/capture/camerathread.cpp \
    $$PWD/src/components/capture/rtspthread.cpp

HEADERS += \
    $$PWD/include/components/RecordingDialog.h \
    $$PWD/include/components/CardWidget.h \
    $$PWD/include/components/WaveFormManager.h \
    $$PWD/include/components/AudioUploader.h \
    $$PWD/include/components/QcustomPlot.h \
    $$PWD/include/components/MonitorWidget.h \
    $$PWD/include/components/capture/camerathread.h \
    $$PWD/include/components/capture/capturethread.h \
    $$PWD/include/components/capture/rtspthread.h
