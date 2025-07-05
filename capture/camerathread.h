#ifndef CameraThread_H
#define CameraThread_H

#include <QThread>
#include <QObject>
#include <QDebug>
#include <QFile>

#include <stddef.h>

#ifdef __arm__
#include "atk_camera.h"
#endif

class CameraThread : public QThread
{
    Q_OBJECT
signals:
    void cameraRestart(bool);
public:
    CameraThread(QObject *parent = nullptr);
    void setFlag(bool flag) {
        Q_UNUSED(flag)
#if __arm__
        camera_quit_flag = flag;
#endif
    }
    void run() override {
        camera_init_success = true;
#if __arm__
        camera_quit_flag = false;
        if (atk_camera_init(cameraId) != 0) {
            camera_init_success = false;
            qDebug() << u8"请检查摄像头连接状况!" << endl;
        }
#endif
    }
    bool camera_init_success = true;

private slots:
    void cameraIdChanged(int m_cameraId) {
        cameraId =  m_cameraId;
        msleep(1000);
        this->start();
        emit cameraRestart(true);
    }

private:
    int cameraId = 0;
};

#endif // CameraThread_H
