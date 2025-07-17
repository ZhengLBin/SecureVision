/******************************************************************
Copyright © Deng Zhimao Co., Ltd. 2021-2030. All rights reserved.
* @projectName   desktop
* @brief         capture_thread.h
* @author        Deng Zhimao
* @email         dengzhimao@alientek.com
* @link          www.openedv.com
* @date          2021-09-22
*******************************************************************/
#ifndef CAPTURE_THREAD_H
#define CAPTURE_THREAD_H
#include "camerathread.h"

#include <QThread>
#include <QDebug>
#include <QImage>
#include <QByteArray>
#include <QBuffer>
#include <QTime>

class CaptureThread : public QThread
{
    Q_OBJECT

signals:
    void resultReady(QImage);
    void sendImage(QImage);
    void cameraIdChanged(int);

public:
    bool startFlag = false;

private:
    bool photoGraphFlag = false;
    CameraThread *m_CameraThread;

public:
    CaptureThread(QObject *parent = nullptr) {
        Q_UNUSED(parent);
        m_CameraThread = new CameraThread(this);
        m_CameraThread->start();
        connect(this, SIGNAL(cameraIdChanged(int)),
                m_CameraThread, SLOT(cameraIdChanged(int)));
        connect(m_CameraThread, SIGNAL(cameraRestart(bool)),
                this, SLOT(startThread(bool)));
    }

    ~CaptureThread() override{
        m_CameraThread->setFlag(true);
        m_CameraThread->quit();
        m_CameraThread->wait();
        delete m_CameraThread;
       // 等待 CameraThread 完全退出

    }

    void setPhotoGraphFlag(bool photo) {
        photoGraphFlag = photo;
    }

    void setThreadStart(bool start) {
        startFlag = start;
    }

    void run() override {
        msleep(800);
#ifdef __arm__
        while (startFlag && m_CameraThread->camera_init_success) {
            msleep(33);
            CameraFrame *frame = GetCameraMediaBuffer();
            if (frame) {
                QImage qImage((unsigned char *)frame->file, 720, 1280, QImage::Format_RGB888);
                
                // 添加旋转逻辑
                QTransform transform;
                transform.rotate(-270);  // 逆时针旋转90°
                QImage rotatedImage = qImage.transformed(transform);

                emit resultReady(rotatedImage);

                if (photoGraphFlag) {
                    if (rotatedImage.isNull()) {
                        delete frame;
                        return;
                    }
                    photoGraphFlag = false;
                    emit sendImage(rotatedImage);
                }
            }
            delete frame;
        }
#endif
    }

public slots:
    void changeCameraId(int cameraId) {
        setThreadStart(false);
        msleep(100);
        m_CameraThread->setFlag(true);
        emit cameraIdChanged(cameraId);
    }

    void startThread(bool start) {
        msleep(1000);
        startFlag = start;
        this->start();
    }
};

#endif // CAPTURE_THREAD_H
