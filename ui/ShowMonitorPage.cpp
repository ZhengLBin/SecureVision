#include "RecordingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QApplication>
#include <QDir>
#include <QProcess>

extern "C" {
#include <libavformat/avformat.h>
}
#include "ShowMonitorPage.h"
#include "../main/secureVision.h"

// ============================================================================
// PerformanceMonitor实现（简化版本）
// ============================================================================

PerformanceMonitor::PerformanceMonitor(QObject* parent)
    : QObject(parent)
{
    m_metricsTimer = new QTimer(this);
    connect(m_metricsTimer, &QTimer::timeout, this, &PerformanceMonitor::calculateFPS);
    m_metricsTimer->start(1000); // 每秒更新一次

    // 系统监控定时器
    QTimer* sysTimer = new QTimer(this);
    connect(sysTimer, &QTimer::timeout, this, &PerformanceMonitor::updateSystemMetrics);
    sysTimer->start(2000); // 每2秒更新系统指标

    // 每分钟输出性能指标的debug定时器
    QTimer* debugTimer = new QTimer(this);
    connect(debugTimer, &QTimer::timeout, this, &PerformanceMonitor::debugOutputMetrics);
    debugTimer->start(60000); // 每分钟输出一次
}

void PerformanceMonitor::recordFrame(const QImage& frame)
{
    QDateTime now = QDateTime::currentDateTime();
    m_frameTimestamps.enqueue(now);

    // 保持最近5秒的时间戳
    while (!m_frameTimestamps.isEmpty() &&
           m_frameTimestamps.first().msecsTo(now) > 5000) {
        m_frameTimestamps.dequeue();
    }

    // 更新指标
    m_metrics.resolution = frame.size();
    m_metrics.colorFormat = "RGB888";
    m_totalFrames++;

    // 计算数据量 (RGB888 = 3 bytes per pixel)
    double frameSize = frame.width() * frame.height() * 3 / (1024.0 * 1024.0); // MB
    m_totalDataReceived += frameSize;

    // 检测延迟
    if (m_lastFrameTime.isValid()) {
        m_metrics.frameLatency = m_lastFrameTime.msecsTo(now);

        // 检测丢帧
        double expectedInterval = 1000.0 / 30.0; // 假设30fps
        if (m_metrics.frameLatency > expectedInterval * 1.5) {
            m_droppedFrames++;
            m_metrics.droppedFrames = m_droppedFrames;
        }
    }
    m_lastFrameTime = now;
}

void PerformanceMonitor::calculateFPS()
{
    if (m_frameTimestamps.size() < 2) return;

    QDateTime now = QDateTime::currentDateTime();

    // 计算当前FPS（最近1秒内的帧数）
    int recentFrames = 0;
    for (auto it = m_frameTimestamps.rbegin(); it != m_frameTimestamps.rend(); ++it) {
        if (it->msecsTo(now) <= 1000) {
            recentFrames++;
        } else {
            break;
        }
    }
    m_metrics.currentFPS = recentFrames;

    // 计算平均FPS（最近5秒）
    m_metrics.avgFPS = m_frameTimestamps.size() / 5.0;

    // 计算码率 (Mbps)
    static double lastDataReceived = 0;
    double dataInLastSecond = m_totalDataReceived - lastDataReceived;
    m_metrics.bitrate = dataInLastSecond * 8; // MB/s to Mbps
    lastDataReceived = m_totalDataReceived;

    emit metricsUpdated(m_metrics);
}

void PerformanceMonitor::updateSystemMetrics()
{
    m_metrics.cpuUsage = getCPUUsage();
    m_metrics.memoryUsage = getMemoryUsage();

    // 更新缓冲区水平（模拟）
    m_metrics.bufferLevel = qrand() % 20 + 80; // 模拟缓冲区使用率80-100%
}

void PerformanceMonitor::debugOutputMetrics()
{
    qDebug() << "=== Performance Metrics Debug Output ===";
    qDebug() << QString("FPS: Current=%1, Average=%2")
                    .arg(m_metrics.currentFPS, 0, 'f', 1)
                    .arg(m_metrics.avgFPS, 0, 'f', 1);
    qDebug() << QString("Resolution: %1x%2 (%3)")
                    .arg(m_metrics.resolution.width())
                    .arg(m_metrics.resolution.height())
                    .arg(m_metrics.colorFormat);
    qDebug() << QString("Bitrate: %1 Mbps").arg(m_metrics.bitrate, 0, 'f', 1);
    qDebug() << QString("Latency: %1 ms").arg(m_metrics.frameLatency);
    qDebug() << QString("Dropped Frames: %1").arg(m_metrics.droppedFrames);
    qDebug() << QString("CPU Usage: %1%").arg(m_metrics.cpuUsage, 0, 'f', 1);
    qDebug() << QString("Memory Usage: %1 MB").arg(m_metrics.memoryUsage, 0, 'f', 0);
    qDebug() << QString("Buffer Level: %1%").arg(m_metrics.bufferLevel, 0, 'f', 0);
    qDebug() << "========================================";
}

double PerformanceMonitor::getCPUUsage()
{
    // 简化的CPU使用率获取（实际项目中需要读取/proc/stat）
    static int counter = 0;
    counter = (counter + 1) % 100;
    return 15.0 + (counter % 30); // 模拟15-45%的CPU使用率
}

double PerformanceMonitor::getMemoryUsage()
{
    // 简化的内存使用获取
    return 256.0 + (qrand() % 100); // 模拟256-356MB使用量
}

// ============================================================================
// ShowMonitorPage实现（完整保留原有功能 + 性能监控）
// ============================================================================

ShowMonitorPage::ShowMonitorPage(const int type, const QString& rtspUrl,
                                 CaptureThread* mipiThread,
                                 RtspThread* rtspThread1,
                                 RtspThread* rtspThread2,
                                 USBCaptureThread* usbThread,
                                 QWidget *parent)
    : QWidget(parent), captureThread(mipiThread), rtspThread1(rtspThread1),
    rtspThread2(rtspThread2), usbCaptureThread(usbThread), m_type(type),
    aiDetectionThread(nullptr)
{
    initUI();
    initConnections();
    setupAIComponents();
    setStreamUrl(rtspUrl);

    // 获取AI线程引用（保持原有逻辑）
    if (SecureVision* mainWindow = qobject_cast<SecureVision*>(parent)) {
        aiDetectionThread = mainWindow->getAIThread();
        qDebug() << "AI Detection Thread obtained:" << (aiDetectionThread != nullptr);
    } else {
        qDebug() << "Failed to get parent SecureVision window";
    }

    // 初始化性能监控（新增）
    m_performanceMonitor = new PerformanceMonitor(this);
    connect(m_performanceMonitor, &PerformanceMonitor::metricsUpdated,
            this, &ShowMonitorPage::onPerformanceMetricsUpdated);
}

ShowMonitorPage::~ShowMonitorPage()
{
    // 原有的析构逻辑（保持不变）
    if (captureThread) {
        captureThread->setThreadStart(false);
        delete captureThread;
        captureThread = nullptr;
    }

    if (rtspThread1) {
        rtspThread1->setThreadStart(false);
        delete rtspThread1;
        rtspThread1 = nullptr;
    }

    if (rtspThread2) {
        rtspThread2->setThreadStart(false);
        delete rtspThread2;
        rtspThread2 = nullptr;
    }

    if (usbCaptureThread) {
        usbCaptureThread->setThreadStart(false);
        delete usbCaptureThread;
        usbCaptureThread = nullptr;
    }
}

void ShowMonitorPage::initUI()
{
    // 主布局（充满整个窗口）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 初始化所有UI组件
    initStreamLabel();
    initVideoLabel();
    initPerformanceLabel();  // 性能标签将覆盖在视频上
    initButtonArea();

    // 添加到主布局（性能标签不添加到布局中，因为要覆盖显示）
    mainLayout->addWidget(streamLabel, 0, Qt::AlignTop);
    mainLayout->addStretch();
    mainLayout->addWidget(videoLabel, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonContainer, 0, Qt::AlignBottom);

    setLayout(mainLayout);
}

void ShowMonitorPage::initStreamLabel()
{
    // 原有逻辑保持不变
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

void ShowMonitorPage::initPerformanceLabel()
{
    // 性能标签作为videoLabel的子组件，覆盖显示
    performanceLabel = new QLabel(videoLabel);
    performanceLabel->setStyleSheet(
        "QLabel {"
        "color: #00FF00;"
        "font-family: 'Consolas', 'Monaco', monospace;"
        "font-size: 12px;"
        "background-color: rgba(0, 0, 0, 180);"
        "padding: 8px;"
        "border-radius: 3px;"
        "}"
        );
    performanceLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    performanceLabel->setText("Performance metrics loading...");

    // 设置绝对定位，覆盖在视频左上角
    performanceLabel->move(10, 10);
    performanceLabel->resize(350, 200); // 设置固定大小
    performanceLabel->raise(); // 确保在最上层显示
}

void ShowMonitorPage::initVideoLabel()
{
    // 原有逻辑保持不变
    videoLabel = new QLabel(this);
    videoLabel->setFixedSize(1280, 720);
    videoLabel->setStyleSheet("background-color: black;");
    videoLabel->setAlignment(Qt::AlignCenter);

    // 确保videoLabel可以包含子组件
    videoLabel->setAttribute(Qt::WA_TransparentForMouseEvents, false);
}

void ShowMonitorPage::setupAIComponents()
{
    // 原有AI组件初始化逻辑（保持不变）
    m_visualizer = new DetectionVisualizer(this);

    m_recordManager = new RecordManager(this);
    connect(m_recordManager, &RecordManager::recordingStateChanged,
            this, &ShowMonitorPage::onRecordingStateChanged);

    m_aiControlWidget = new AIControlWidget(this);
    m_aiControlWidget->hide();

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
    // 修改原有按钮区域，添加性能按钮
    buttonContainer = new QWidget(this);
    buttonContainer->setAttribute(Qt::WA_TranslucentBackground);
    buttonContainer->setStyleSheet("background:transparent;");

    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 20);
    buttonLayout->setSpacing(20);

    // 原有按钮初始化
    initBackButton();
    initAiButton();
    initAiControlButton();

    // 新增性能按钮
    performanceButton = new QPushButton("性能", buttonContainer);
    performanceButton->setCheckable(true);
    performanceButton->setChecked(true);
    performanceButton->setFixedSize(150, 60);
    performanceButton->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(0, 200, 100, 200);"
        "color: white; font-size: 20px; border-radius: 10px;"
        "}"
        "QPushButton:checked { background-color: rgba(0, 255, 127, 200); }"
        );

    buttonLayout->addStretch();
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(performanceButton);  // 新增
    buttonLayout->addWidget(aiButton);
    buttonLayout->addWidget(aiControlButton);
    buttonLayout->addStretch();

    buttonContainer->setLayout(buttonLayout);
}

void ShowMonitorPage::initBackButton()
{
    // 原有逻辑保持不变
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
    // 原有逻辑保持不变
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
    // 原有逻辑保持不变
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
    // 原有逻辑保持不变
    m_type = type;
}

void ShowMonitorPage::toggleAI(bool enabled)
{
    // 原有AI切换逻辑（保持不变）
    aiEnabled = enabled;
    aiButton->setText(enabled ? "关闭AI检测" : "开启AI检测");

    if (aiDetectionThread) {
        if (enabled) {
            qDebug() << "UI: Starting face recognition test instead of motion detection";
            aiDetectionThread->runSimpleFaceTest();
        } else {
            qDebug() << "UI: Face recognition test finished";
        }
    }
}

void ShowMonitorPage::initConnections()
{
    // 原有连接（保持不变）
    connect(backButton, &QPushButton::clicked, this, &ShowMonitorPage::backToMonitorList);
    connect(aiButton, &QPushButton::toggled, this, &ShowMonitorPage::toggleAI);
    connect(aiControlButton, &QPushButton::clicked, this, &ShowMonitorPage::toggleAIControl);

    // 新增连接
    connect(performanceButton, &QPushButton::toggled, this, &ShowMonitorPage::togglePerformanceDisplay);
}

void ShowMonitorPage::connectMipiThread()
{
    // 修改原有逻辑，添加性能监控
    if (captureThread) {
        disconnectThreads();

        connect(captureThread, &CaptureThread::resultReady, this, [=](QImage image) {
            // 1. 记录帧用于性能统计（新增）
            m_performanceMonitor->recordFrame(image);

            // 2. 更新显示图像（原有逻辑）
            updateDisplayImage(image);

            // 3. 发送到AI检测（原有逻辑）
            if (aiEnabled && aiDetectionThread) {
                if (!image.isNull()) {
                    aiDetectionThread->addFrame(image);
                }
            }
        });

        qDebug() << "MIPI thread connected with performance monitoring";
    }
}

void ShowMonitorPage::updateDisplayImage(const QImage& originalImage)
{
    // 原有显示逻辑（保持不变）
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
    // 原有逻辑保持不变
    if (rtspThread1) {
        disconnectThreads();

        connect(rtspThread1, &RtspThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });

        qDebug() << "RTSP thread1 connected to videoLabel";
    }
}

void ShowMonitorPage::connectRtspThread2()
{
    // 原有逻辑保持不变
    if (rtspThread2) {
        disconnectThreads();

        connect(rtspThread2, &RtspThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });

        qDebug() << "RTSP thread2 connected to videoLabel";
    }
}

void ShowMonitorPage::connectUSBThread()
{
    // 原有逻辑保持不变
    if (usbCaptureThread) {
        disconnectThreads();
        connect(usbCaptureThread, &USBCaptureThread::resultReady, this, [=](QImage image) {
            videoLabel->setPixmap(QPixmap::fromImage(image).scaled(videoLabel->size(), Qt::KeepAspectRatio));
        });
    }
}

void ShowMonitorPage::disconnectThreads()
{
    // 原有逻辑保持不变
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
    // 原有逻辑保持不变
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

// 原有AI相关槽函数（保持不变）
void ShowMonitorPage::onDetectionResult(const DetectionResult& result)
{
    m_lastDetectionResult = result;

    if (m_aiControlWidget) {
        m_aiControlWidget->updateDetectionStatus(result);
    }

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
    // 原有AI控制切换逻辑（保持不变）
    m_aiControlVisible = !m_aiControlVisible;

    if (m_aiControlVisible) {
        QPoint topLeft = mapToGlobal(QPoint(200, 100));
        m_aiControlWidget->move(topLeft);
        m_aiControlWidget->show();
        m_aiControlWidget->raise();
    } else {
        m_aiControlWidget->hide();
    }
}

// ============================================================================
// 新增的性能监控相关方法（仅此部分是新的）
// ============================================================================

void ShowMonitorPage::onPerformanceMetricsUpdated(const PerformanceMetrics& metrics)
{
    if (m_showPerformanceMetrics) {
        updatePerformanceDisplay(metrics);
    }
}

void ShowMonitorPage::updatePerformanceDisplay(const PerformanceMetrics& metrics)
{
    QString perfText = QString(
                           "┌── 视频性能 ──────────────────────────┐\n"
                           "│ FPS: %1 (平均: %2)                    │\n"
                           "│ 分辨率: %3×%4 %5                      │\n"
                           "│ 码率: %6 Mbps                        │\n"
                           "│ 延迟: %7 ms                          │\n"
                           "│ 丢帧: %8                             │\n"
                           "├── 系统资源 ──────────────────────────┤\n"
                           "│ CPU: %9%                             │\n"
                           "│ 内存: %10 MB                         │\n"
                           "│ 缓冲区: %11%                         │\n"
                           "└─────────────────────────────────────┘"
                           ).arg(QString::number(metrics.currentFPS, 'f', 1))
                           .arg(QString::number(metrics.avgFPS, 'f', 1))
                           .arg(metrics.resolution.width())
                           .arg(metrics.resolution.height())
                           .arg(metrics.colorFormat)
                           .arg(QString::number(metrics.bitrate, 'f', 1))
                           .arg(metrics.frameLatency)
                           .arg(metrics.droppedFrames)
                           .arg(QString::number(metrics.cpuUsage, 'f', 1))
                           .arg(QString::number(metrics.memoryUsage, 'f', 0))
                           .arg(QString::number(metrics.bufferLevel, 'f', 0));

    performanceLabel->setText(perfText);
}

void ShowMonitorPage::togglePerformanceDisplay()
{
    m_showPerformanceMetrics = performanceButton->isChecked();
    performanceLabel->setVisible(m_showPerformanceMetrics);
}
