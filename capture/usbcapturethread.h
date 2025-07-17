#ifndef USBCAPTURETHREAD_H
#define USBCAPTURETHREAD_H

#include <QThread>
#include <QImage>

class USBCaptureThread : public QThread
{
    Q_OBJECT

public:
    explicit USBCaptureThread(QObject *parent = nullptr);
    ~USBCaptureThread() override;

    void setDevice(const QString &device);
    void setThreadStart(bool start);

signals:
    void resultReady(const QImage &image);

protected:
    void run() override;

private:
    QString m_device;
    bool m_start = false;
};

#endif // USBCAPTURETHREAD_H
