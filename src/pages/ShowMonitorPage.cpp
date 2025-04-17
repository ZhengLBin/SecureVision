#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

extern "C" {
#include <libavformat/avformat.h>
}
#include "../../include/pages/ShowMonitorPage.h"

ShowMonitorPage::ShowMonitorPage(const int type, const QString& rtspUrl, QWidget *parent)
     : QWidget(parent), m_type(type)
{
    initUI();
    initConnections();
    qDebug() << "ShowMonitorPage construct";
    if (rtspUrl == "IP 01") {
        m_rtspUrl = "rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/101?transportmode=unicast";
    } else if (rtspUrl == "IP 02") {
        m_rtspUrl = "rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/201?transportmode=unicast";
    }
    if (m_type == 0) {
        initMipiCaptureThread();
    } else if (m_type == 1) {
        initRtspCaptureThread();
    }
}

ShowMonitorPage::~ShowMonitorPage()
{

    // 只有在初始化了 captureThread 后才执行删除
    if (isMIPICaptureThreadInitialized) {
        captureThread->setThreadStart(false);
        delete captureThread;         // 删除线程
        captureThread = nullptr;      // 置空指针
        isMIPICaptureThreadInitialized = false;  // 重置标志
    }

    if (isrtspCaptureThreadInitialized) {
        rtspThread->setThreadStart(false);
        delete rtspThread;         // 删除线程
        rtspThread = nullptr;      // 置空指针
        isrtspCaptureThreadInitialized = false;  // 重置标志
    }
}


void ShowMonitorPage::initUI()
{
    // 主布局（充满整个窗口）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 初始化顶部信息标签
    initStreamLabel();
    // 初始化视频显示区域
    initVideoLabel();
    // 初始化底部按钮区域
    initButtonArea();

    // 添加到主布局
    mainLayout->addWidget(streamLabel, 0, Qt::AlignTop);
    mainLayout->addStretch();
    mainLayout->addWidget(videoLabel, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonContainer, 0, Qt::AlignBottom);
    setLayout(mainLayout);
}

void ShowMonitorPage::initStreamLabel()
{
    streamLabel = new QLabel(this);
    streamLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 18px;"
        "background-color: rgba(0, 0, 0, 120);"
        "padding: 10px;"
        "border-radius: 5px;"
        "}");
    streamLabel->setAlignment(Qt::AlignCenter);
    streamLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void ShowMonitorPage::initVideoLabel()
{
    videoLabel = new QLabel(this);
    videoLabel->setFixedSize(1280, 720);  // 你摄像头分辨率是多少就设多大
    videoLabel->setStyleSheet("background-color: black;");
    videoLabel->setAlignment(Qt::AlignCenter);
}

void ShowMonitorPage::initButtonArea()
{
    // 底部按钮容器
    buttonContainer = new QWidget(this);
    buttonContainer->setAttribute(Qt::WA_TranslucentBackground);
    buttonContainer->setStyleSheet("background:transparent;");

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 20);
    buttonLayout->setSpacing(20);

    // 初始化返回按钮
    initBackButton();

    buttonLayout->addStretch();
    buttonLayout->addWidget(backButton);
    buttonLayout->addStretch();
    buttonContainer->setLayout(buttonLayout);
}

void ShowMonitorPage::initBackButton()
{
    backButton = new QPushButton("返回", buttonContainer);
    backButton->setFixedSize(200, 60);
    backButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0, 123, 255, 200);"
        "color: white;"
        "font-size: 24px;"
        "border-radius: 10px;"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(0, 86, 179, 200);"
        "}");
}

void ShowMonitorPage::initConnections()
{
    connect(backButton, &QPushButton::clicked, this, &ShowMonitorPage::backToMonitorList);
}

void ShowMonitorPage::initMipiCaptureThread()
{
    if (!isMIPICaptureThreadInitialized) {
        captureThread = new CaptureThread(this);
        connect(captureThread, &CaptureThread::resultReady, this, [=](QImage image){
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });
        captureThread->setThreadStart(true);
        captureThread->start();

        isMIPICaptureThreadInitialized = true;  // 标志设置为已初始化
    }
}

void ShowMonitorPage::initRtspCaptureThread()
{
    qDebug() << "initRtspCaptureThread";
    if (!isrtspCaptureThreadInitialized) {
        rtspThread = new RtspThread(this);
        rtspThread->setRtspUrl(m_rtspUrl);
        connect(rtspThread, &RtspThread::resultReady, this, [=](QImage image){
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });
        rtspThread->setThreadStart(true);
        rtspThread->start();
        isrtspCaptureThreadInitialized = true;
        qDebug() << "initRtspCaptureThread successful";
    }
}

void ShowMonitorPage::setStreamUrl(const QString& url)
{
    streamLabel->setText(QString("摄像头实时预览 [%1]").arg(url));


}
