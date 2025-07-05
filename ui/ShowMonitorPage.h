#ifndef SHOWMONITORPAGE_H
#define SHOWMONITORPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QTimer>

#include "../capture/capturethread.h"
#include "../capture/rtspthread.h"

class ShowMonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShowMonitorPage(const int type, const QString& rtspUrl,
                            CaptureThread* mipiThread,
                            RtspThread* rtspThread1,
                            RtspThread* rtspThread2,
                            QWidget *parent = nullptr);
    ~ShowMonitorPage();

    void setStreamUrl(const QString& url);
    void setType(int type);

signals:
    void backToMonitorList();  // 返回按钮信号

private:
    // 初始化方法
    void initUI();
    void initStreamLabel();
    void initVideoLabel();
    void initButtonArea();
    void initBackButton();
    void connectMipiThread();
    void connectRtspThread1();
    void connectRtspThread2();
    void disconnectThreads();
    

    void initConnections();
    
    // void initMipiCaptureThread();
    // void initRtspCaptureThread();

    // UI控件
    QLabel *streamLabel;
    QPushButton *backButton;
    QLabel *videoLabel;
    QWidget *buttonContainer;  // 新增的按钮容器成员变量

    CaptureThread *captureThread;
    RtspThread *rtspThread1;
    RtspThread *rtspThread2;
    int m_type;
    QString m_rtspUrl;
};

#endif // SHOWMONITORPAGE_H
