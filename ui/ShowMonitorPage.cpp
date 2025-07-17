#include "RecordingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>

extern "C" {
#include <libavformat/avformat.h>
}
#include "ShowMonitorPage.h"
#include "../main/secureVision.h"


ShowMonitorPage::ShowMonitorPage(const int type, const QString& rtspUrl, 
                                CaptureThread* mipiThread,
                                RtspThread* rtspThread1,
                                RtspThread* rtspThread2,
                                USBCaptureThread* usbThread,
                                QWidget *parent)
     : QWidget(parent), captureThread(mipiThread), rtspThread1(rtspThread1), rtspThread2(rtspThread2),usbCaptureThread(usbThread), m_type(type), aiDetectionThread(nullptr)
{
    initUI();
    initConnections();
    setupAIComponents();
    setStreamUrl(rtspUrl);

    // 获取AI线程引用
    if (SecureVision* mainWindow = qobject_cast<SecureVision*>(parent)) {
        aiDetectionThread = mainWindow->getAIThread();
        qDebug() << "AI Detection Thread obtained:" << (aiDetectionThread != nullptr);
    } else {
        qDebug() << "Failed to get parent SecureVision window";
    }
}

ShowMonitorPage::~ShowMonitorPage()
{

    // 只有在初始化了 captureThread 后才执行删除
    if (captureThread) {
        captureThread->setThreadStart(false);
        delete captureThread;         // 删除线程
        captureThread = nullptr;      // 置空指针
    }

    if (rtspThread1) {
        rtspThread1->setThreadStart(false);
        delete rtspThread1;         // 删除线程
        rtspThread1 = nullptr;      // 置空指针
    }

    if (rtspThread2) {
        rtspThread2->setThreadStart(false);
        delete rtspThread2;         // 删除线程
        rtspThread2 = nullptr;      // 置空指针
    }

    if (usbCaptureThread) {
        usbCaptureThread->setThreadStart(false);
        delete usbCaptureThread;         // 删除线程
        usbCaptureThread = nullptr;      // 置空指针
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

void ShowMonitorPage::setupAIComponents()
{
    // 初始化可视化器
    m_visualizer = new DetectionVisualizer(this);

    // 初始化录像管理器
    m_recordManager = new RecordManager(this);
    connect(m_recordManager, &RecordManager::recordingStateChanged,
            this, &ShowMonitorPage::onRecordingStateChanged);

    // 初始化AI控制面板
    m_aiControlWidget = new AIControlWidget(this);
    m_aiControlWidget->hide(); // 默认隐藏

    // 连接控制面板信号
    connect(m_aiControlWidget, &AIControlWidget::aiToggled,
            this, &ShowMonitorPage::onAIToggled);
    connect(m_aiControlWidget, &AIControlWidget::visualizationToggled,
            this, &ShowMonitorPage::onVisualizationToggled);
    connect(m_aiControlWidget, &AIControlWidget::recordingToggled,
            this, &ShowMonitorPage::onRecordingToggled);
    connect(m_aiControlWidget, &AIControlWidget::configChanged,
            this, &ShowMonitorPage::onConfigChanged);
}

void ShowMonitorPage::initButtonArea()
{
    // 修改原有的按钮初始化，添加AI控制按钮
    buttonContainer = new QWidget(this);
    buttonContainer->setAttribute(Qt::WA_TranslucentBackground);
    buttonContainer->setStyleSheet("background:transparent;");

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 20);
    buttonLayout->setSpacing(20);

    // 原有按钮
    initBackButton();
    initAiButton();
    initAiControlButton();

    buttonLayout->addStretch();
    buttonLayout->addWidget(backButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(aiButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(aiControlButton);
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

void ShowMonitorPage::initAiButton()
{
    aiButton = new QPushButton("开启AI检测", buttonContainer);
    aiButton->setCheckable(true);
    aiButton->setFixedSize(200, 60);
    aiButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(100, 100, 100, 200);"
        "color: white;"
        "font-size: 24px;"
        "border-radius: 10px;"
        "}"
        "QPushButton:checked {"
        "background-color: rgba(0, 200, 0, 200);"
        "}");
}

void ShowMonitorPage::initAiControlButton()
{
    // 新增AI控制按钮
    aiControlButton = new QPushButton("AI设置", buttonContainer);
    aiControlButton->setFixedSize(150, 60);
    aiControlButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(100, 150, 255, 200);"
        "color: white;"
        "font-size: 20px;"
        "border-radius: 10px;"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(70, 120, 225, 200);"
        "}");
}

void ShowMonitorPage::setType(int type)
{
    m_type = type;
}

void ShowMonitorPage::toggleAI(bool enabled) {
    aiEnabled = enabled;
    aiButton->setText(enabled ? "关闭AI检测" : "开启AI检测");

    if (aiDetectionThread) {
        // 获取当前配置并更新
        AIConfig config = aiDetectionThread->getConfig();
        config.enableAI = enabled;
        aiDetectionThread->setConfig(config);

        // 启动/停止检测
        enabled ? aiDetectionThread->startDetection()
                : aiDetectionThread->stopDetection();
    }
}


void ShowMonitorPage::initConnections()
{
    connect(backButton, &QPushButton::clicked, this, &ShowMonitorPage::backToMonitorList);
    connect(aiButton, &QPushButton::toggled, this, &ShowMonitorPage::toggleAI);
    connect(aiControlButton, &QPushButton::clicked,
            this, &ShowMonitorPage::toggleAIControl);
}

void ShowMonitorPage::connectMipiThread()
{
    if (captureThread) {
        disconnectThreads();

        connect(captureThread, &CaptureThread::resultReady, this, [=](QImage image) {
            // 更新显示图像（包含可视化）
            updateDisplayImage(image);

            // 发送到AI检测（使用原始图像）
            if (aiEnabled && aiDetectionThread) {
                if (!image.isNull()) {
                    aiDetectionThread->addFrame(image);
                }
            }
        });

        qDebug() << "MIPI thread connected with full AI integration";
    }
}

void ShowMonitorPage::updateDisplayImage(const QImage& originalImage)
{
    QImage displayImage = originalImage;

    // 叠加检测结果可视化
    if (m_showDetectionOverlay && !m_lastDetectionResult.timestamp.isNull()) {
        displayImage = m_visualizer->drawDetections(originalImage, m_lastDetectionResult);
    }

    // 显示到界面
    videoLabel->setPixmap(QPixmap::fromImage(displayImage).scaled(
        videoLabel->size(), Qt::KeepAspectRatio));
}

void ShowMonitorPage::connectRtspThread1()
{
    if (rtspThread1) {
        // 先断开之前连接
        disconnectThreads();

        // 连接当前线程到 videoLabel 显示
        connect(rtspThread1, &RtspThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });

        qDebug() << "RTSP thread connected to videoLabel";
    }
}

void ShowMonitorPage::connectRtspThread2()
{
    if (rtspThread2) {
        // 先断开之前连接
        disconnectThreads();

        // 连接当前线程到 videoLabel 显示
        connect(rtspThread2, &RtspThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });

        qDebug() << "RTSP thread connected to videoLabel";
    }
}

void ShowMonitorPage::connectUSBThread()
{
    if (usbCaptureThread) {
        disconnectThreads();
        connect(usbCaptureThread, &USBCaptureThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });
    }
}

void ShowMonitorPage::disconnectThreads()
{
    if (captureThread) {
        disconnect(captureThread, nullptr, this, nullptr);
    }
    if (rtspThread1) {
        disconnect(rtspThread1, nullptr, this, nullptr);
    }
    if (rtspThread2) {
        disconnect(rtspThread2, nullptr, this, nullptr);
    }
    if (usbCaptureThread) {
        disconnect(usbCaptureThread, nullptr, this, nullptr);
    }

}

void ShowMonitorPage::setStreamUrl(const QString& url)
{
    streamLabel->setText(QString("摄像头实时预览 [%1]").arg(url));

    if (m_type == 0) {
        connectMipiThread();
    } else if (m_type == 1) {
        if (url == "IP 01") {
            connectRtspThread1();
        } else if (url == "IP 02") {
            connectRtspThread2();
        }
    } else if (m_type == 2) {
        connectUSBThread();
    }
}

void ShowMonitorPage::onDetectionResult(const DetectionResult& result)
{
    m_lastDetectionResult = result;

    // 更新AI控制面板状态
    if (m_aiControlWidget) {
        m_aiControlWidget->updateDetectionStatus(result);
    }

    // 触发录像管理
    if (result.hasMotion) {
        m_recordManager->onMotionDetected();
    }
}

void ShowMonitorPage::onAIToggled(bool enabled)
{
    toggleAI(enabled);
}

void ShowMonitorPage::onVisualizationToggled(bool enabled)
{
    m_showDetectionOverlay = enabled;
}

void ShowMonitorPage::onRecordingToggled(bool enabled)
{
    m_recordManager->setEnabled(enabled);
}

void ShowMonitorPage::onConfigChanged(const AIConfig& config)
{
    if (aiDetectionThread) {
        aiDetectionThread->setConfig(config);
    }
}

void ShowMonitorPage::onRecordingStateChanged(RecordManager::RecordState state)
{
    if (m_aiControlWidget) {
        m_aiControlWidget->updateRecordingStatus(state);
    }
}

void ShowMonitorPage::toggleAIControl()
{
    m_aiControlVisible = !m_aiControlVisible;

    if (m_aiControlVisible) {
        // 显示AI控制面板
        // 获取主窗口的左上角全局坐标
        QPoint topLeft = mapToGlobal(QPoint(200, 100));
        // 将对话框移动到主窗口的左上角
        m_aiControlWidget->move(topLeft);
        m_aiControlWidget->show();
        m_aiControlWidget->raise();
    } else {
        m_aiControlWidget->hide();
    }
}
