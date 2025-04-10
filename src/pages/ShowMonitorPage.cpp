#include "../../include/pages/ShowMonitorPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

ShowMonitorPage::ShowMonitorPage(const int type, QWidget *parent)
     : QWidget(parent), m_type(type)
{
    initUI();
    initConnections();
    if (m_type == 0) {
        initMipiCaptureThread();
    } else if (m_type == 1) {
        initRtspCaptureThread();
    }
}

ShowMonitorPage::~ShowMonitorPage()
{
    qDebug() << "~ShowMonitorPage deleted" << isCaptureThreadInitialized;

    // 只有在初始化了 captureThread 后才执行删除
    if (isCaptureThreadInitialized) {
        captureThread->setThreadStart(false);
        delete captureThread;         // 删除线程
        captureThread = nullptr;      // 置空指针
        isCaptureThreadInitialized = false;  // 重置标志
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
    if (!isCaptureThreadInitialized) {
        captureThread = new CaptureThread(this);
        connect(captureThread, &CaptureThread::resultReady, this, [=](QImage image){
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });
        captureThread->setThreadStart(true);
        captureThread->start();

        isCaptureThreadInitialized = true;  // 标志设置为已初始化
    }
}

void ShowMonitorPage::initRtspCaptureThread()
{
    qDebug() << "initRtspCaptureThread";
}

void ShowMonitorPage::setStreamUrl(const QString& url)
{
    streamLabel->setText(QString("摄像头实时预览 [%1]").arg(url));
}
