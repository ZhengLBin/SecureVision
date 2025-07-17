#include "usbcapturethread.h"
#include <opencv2/opencv.hpp>

#include<QDebug>

USBCaptureThread::USBCaptureThread(QObject *parent)
    : QThread(parent)
{
}

USBCaptureThread::~USBCaptureThread()
{
    m_start = false;
    wait();
}

void USBCaptureThread::setDevice(const QString &device)
{
    m_device = device;
}

void USBCaptureThread::setThreadStart(bool start)
{
    m_start = start;
}

void USBCaptureThread::run()
{
    cv::VideoCapture cap(m_device.toStdString());
    if (!cap.isOpened()) {
        qDebug() << "Failed to open USB camera:" << m_device;
        return;
    }

    cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    while (m_start) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            qDebug() << "Failed to read frame from USB camera";
            break;
        }

        if (!frame.empty()) {
            cv::Mat rgbFrame;
            cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

            QImage image(
                rgbFrame.data,
                rgbFrame.cols,
                rgbFrame.rows,
                rgbFrame.step,
                QImage::Format_RGB888
                );
            emit resultReady(image.copy());

        }

        QThread::msleep(30); // 控制帧率
    }

    cap.release();
}
