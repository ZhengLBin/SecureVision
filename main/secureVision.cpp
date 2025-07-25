#include "secureVision.h"
#include "../ui/MonitorListPage.h"
#include "../ui/DetectionListPage.h"
#include "../ui/VideoDetectionPage.h"
#include "../ui/AudioDetectionPage.h"
#include "../ui/ShowMonitorPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

SecureVision::SecureVision(QWidget* parent)
    : QMainWindow(parent),
      monitorPage(std::make_unique<MonitorListPage>(this)),
      anomalyPage(std::make_unique<QWidget>(this)),
      detectionPage(std::make_unique<DetectionListPage>(this)),
      videoPage(std::make_unique<VideoDetectionPage>(this)),
      audioPage(std::make_unique<AudioDetectionPage>(this))
{

    qRegisterMetaType<DetectionResult>("DetectionResult");
    qRegisterMetaType<RecordTrigger>("RecordTrigger");

    initializeWindow();
    setupUi();
    setupGlobalStack();
    setupThread();
    setCentralWidget(globalStack);
}

void SecureVision::initializeWindow()
{
    setWindowTitle("SecureVision");
    setFixedSize(1280, 720);
}

void SecureVision::setupUi()
{
    createLeftNavigationBar();
    setupGlobalStack();
    // 监听 MonitorListPage 的点击信号，进入 ShowMonitorPage
    connect(monitorPage.get(), &MonitorListPage::openShowMonitorPage, this, &SecureVision::showMonitorStream);

}

void SecureVision::createLeftNavigationBar()
{
    leftBar = new QListWidget(this);
    leftBar->setStyleSheet("margin-top: 21px;");
    leftBar->setFixedWidth(230);

    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(20);

    // Logo Section (保持不变)
    QWidget* topWidget = new QWidget();
    QHBoxLayout* topLayout = new QHBoxLayout(topWidget);
    topLayout->setAlignment(Qt::AlignLeft);
    QLabel* artLabel = new QLabel(topWidget);
    artLabel->setPixmap(QPixmap(":/AVDeepfake/appTitle").scaled(160, 35, Qt::KeepAspectRatio));
    topLayout->addWidget(artLabel);
    layout->addWidget(topWidget);

    // Navigation Buttons
    const QString buttonStyle = R"(
        QPushButton {
            text-align: left; padding: 15px; font-size: 32px;
            color: #333; border: none; border-radius: 10px;
            background-color: transparent;
        }
        QPushButton:hover { background-color: #F0F0F0; }
        QPushButton:checked { background-color: #E0E0E0; color: #6A0DAD; }
    )";

    const QVector<QPair<QString, QString>> buttons = {
        {":/AVDeepfake/dashboard", u8"监控"},
        {":/AVDeepfake/history", u8"异常信息"},
        {":/AVDeepfake/voice", u8"篡改检测"}
    };

    for (int i = 0; i < buttons.size(); ++i) {
        QPushButton* btn = new QPushButton();
        btn->setIcon(QIcon(buttons[i].first));
        btn->setText(buttons[i].second);
        btn->setIconSize(QSize(36, 36));
        btn->setStyleSheet(buttonStyle);
        btn->setCheckable(true);
        btn->setProperty("pageIndex", i);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            switchRightPage(i);  // 切换右侧页面
        });

        layout->addWidget(btn);
        if (i == 0) btn->setChecked(true);  // 默认选中第一个
    }

    layout->addStretch();
    leftBar->setLayout(layout);
}

void SecureVision::setupGlobalStack()
{
    globalStack = new QStackedWidget(this);

    // 模式1：正常布局（左侧导航栏 + rightStack）
    normalModeWidget = new QWidget(this);
    rightStack = new QStackedWidget(normalModeWidget);

    // 添加右侧页面
    rightStack->addWidget(monitorPage.get());   // 索引 0: 监控页
    rightStack->addWidget(anomalyPage.get());   // 索引 1: 异常信息页
    rightStack->addWidget(detectionPage.get()); // 索引 2: 篡改检测页

    // 连接检测页面的卡片点击信号
    connect(detectionPage.get(), &DetectionListPage::cardClicked, this, &SecureVision::handleDetectClick);

    // 布局模式1
    QHBoxLayout* normalLayout = new QHBoxLayout(normalModeWidget);
    normalLayout->setContentsMargins(0, 0, 0, 0);
    normalLayout->addWidget(leftBar);
    normalLayout->addWidget(rightStack);
    normalModeWidget->setLayout(normalLayout);

    // 模式2：全屏检测页面
    globalStack->addWidget(normalModeWidget);  // 索引 0
    globalStack->addWidget(videoPage.get());   // 索引 1: 视频检测页
    globalStack->addWidget(audioPage.get());   // 索引 2: 音频检测页

    
    // 连接返回信号
    connect(videoPage.get(), &VideoDetectionPage::returnRequested, this, &SecureVision::returnToDetectionPage);
    connect(audioPage.get(), &AudioDetectionPage::returnRequested, this, &SecureVision::returnToDetectionPage);
    //connect(monitorPageShow.get(), &ShowMonitorPage::returnRequested, this, &SecureVision::returnToMonitorListPage);
}

void SecureVision::setupThread () {
    mipiThread = new CaptureThread(this);
    mipiThread->setThreadStart(true);
    mipiThread->start();

    rtspThread1 = new RtspThread(this);
    rtspThread1->setRtspUrl("rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/101?transportmode=unicast"); // 后续可 set
    rtspThread1->setThreadStart(true);
    rtspThread1->start();

    rtspThread2 = new RtspThread(this);
    rtspThread2->setRtspUrl("rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/201?transportmode=unicast"); // 后续可 set
    rtspThread2->setThreadStart(true);
    rtspThread2->start();

    usbThread = new USBCaptureThread(this);
    usbThread->setDevice("/dev/video45");
    usbThread->setThreadStart(true);
    usbThread->start();

    setupAIThread();
}

void SecureVision::setupAIThread()
{
    aiThread = new AIDetectionThread(this);

    // 设置默认AI配置
    aiConfig.enableAI = false;  // 默认关闭，通过UI开启
    aiConfig.enableMotionDetect = true;
    aiConfig.motionThreshold = 0.3f;
    aiConfig.skipFrames = 2;  // 每3帧检测一次

    aiThread->setConfig(aiConfig);
    connectAISignals();

    qDebug() << "AI Thread initialized";
}

void SecureVision::connectAISignals()
{
    connect(aiThread, &AIDetectionThread::detectionResult,
            this, &SecureVision::onDetectionResult);
    connect(aiThread, &AIDetectionThread::recordTrigger,
            this, &SecureVision::onRecordTrigger);
}

void SecureVision::onDetectionResult(const DetectionResult& result)
{
    // 处理人脸检测结果
    if (result.hasFaceDetection) {
        qDebug() << QString("Face detection: %1 faces detected, %2 recognized")
        .arg(result.totalFaceCount)
            .arg(result.recognizedFaceCount);

        // 更新界面显示人脸检测结果
        updateFaceDetectionStatus(result);
    }

    // 现有的运动检测处理...
    if (result.hasMotion) {
        qDebug() << "Motion detected at:" << result.timestamp;
    }
}

void SecureVision::updateFaceDetectionStatus(const DetectionResult& result)
{
    // 更新状态显示（可以添加到状态栏或其他UI组件）
    QString statusText = QString("人脸: %1/%2")
                             .arg(result.recognizedFaceCount)
                             .arg(result.totalFaceCount);

    // TODO: 更新到具体的UI组件
    // statusLabel->setText(statusText);
}

void SecureVision::openFaceRegistration()
{
    if (!aiThread || !aiThread->isRunning()) {
        QMessageBox::warning(this, "错误", "AI检测线程未运行");
        return;
    }

    // 获取人脸识别管理器（需要在AIDetectionThread中添加相应接口）
    auto faceManager = aiThread->getFaceRecognitionManager();
    if (!faceManager) {
        QMessageBox::warning(this, "错误", "人脸识别功能未初始化");
        return;
    }

    FaceRegistrationDialog dialog(faceManager, this);

    // 如果有实时画面，传递给对话框用于捕获
    if (mipiThread && mipiThread->isRunning()) {
        QImage currentFrame = mipiThread->getCurrentFrame();  // 需要在CaptureThread中实现
        dialog.setLiveImage(currentFrame);
    }

    connect(&dialog, &FaceRegistrationDialog::faceRegistered,
            this, [this](const QString& name) {
                qDebug() << "Face registered successfully:" << name;
                // 可以更新UI显示已注册人脸列表
            });

    dialog.exec();
}

void SecureVision::onRecordTrigger(RecordTrigger trigger, const QImage& frame)
{
    // 暂时只打印调试信息
    qDebug() << "Record triggered by:" << (int)trigger;
    // TODO: 下个阶段实现录制功能
}

void SecureVision::switchRightPage(int index)
{
    rightStack->setCurrentIndex(index);
}

void SecureVision::showMonitorStream(const QString& rtspUrl, const int m_type)
{
    if (!showMonitorPage) {
        showMonitorPage = std::make_unique<ShowMonitorPage>(m_type, rtspUrl, mipiThread, rtspThread1, rtspThread2, usbThread, this);
        globalStack->addWidget(showMonitorPage.get());

        // 监听 ShowMonitorPage 的返回信号
        connect(showMonitorPage.get(), &ShowMonitorPage::backToMonitorList, this, &SecureVision::returnToMonitorList);
    } else {
        showMonitorPage->setType(m_type); // 切换显示内容
    }
    showMonitorPage->setStreamUrl(rtspUrl);  // 传递 RTSP 地址
    globalStack->setCurrentWidget(showMonitorPage.get());  // 切换页面
}


void SecureVision::handleDetectClick(int index)
{
    globalStack->setCurrentIndex(index + 1);
}


void SecureVision::returnToDetectionPage()
{
    // 返回正常布局并切换到篡改检测页
    globalStack->setCurrentIndex(0);
    rightStack->setCurrentIndex(2);
}

void SecureVision::returnToMonitorList()
{
    globalStack->setCurrentIndex(0);
    rightStack->setCurrentIndex(0);
    showMonitorPage->hide(); 
    // if (showMonitorPage) {
    //     globalStack->removeWidget(showMonitorPage.get());

    //     // 延迟释放（保证信号处理完后再释放）
    //     auto ptr = showMonitorPage.release();
    //     QTimer::singleShot(0, ptr, [ptr]() { delete ptr; });
    // }
}


SecureVision::~SecureVision() = default;
