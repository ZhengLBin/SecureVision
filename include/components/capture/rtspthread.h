#ifndef RTSPTHREAD_H
#define RTSPTHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <atomic>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class RtspThread : public QThread
{
    Q_OBJECT
public:
    explicit RtspThread(QObject *parent = nullptr);
    ~RtspThread() override;

    void setRtspUrl(const QString &url);
    void setThreadStart(bool start);

signals:
    void resultReady(const QImage &image);

protected:
    void run() override;

private:
    QString m_url;
    std::atomic<bool> m_running;
};

#endif // RTSPTHREAD_H
