#ifndef SHOWMONITORPAGE_H
#define SHOWMONITORPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include <QTimer>

#include "../components/capturethread.h"

class ShowMonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShowMonitorPage(const int type, QWidget *parent = nullptr);
    ~ShowMonitorPage();

    void setStreamUrl(const QString& url);

signals:
    void backToMonitorList();  // 返回按钮信号

private:
    // 初始化方法
    void initUI();
    void initStreamLabel();
    void initVideoLabel();
    void initButtonArea();
    void initBackButton();

    void initConnections();
    
    void initMipiCaptureThread();
    void initRtspCaptureThread();

    // UI控件
    QLabel *streamLabel;
    QPushButton *backButton;
    QLabel *videoLabel;
    QWidget *buttonContainer;  // 新增的按钮容器成员变量

    bool isCaptureThreadInitialized = false; 
    CaptureThread *captureThread;
    int m_type;
};

#endif // SHOWMONITORPAGE_H
