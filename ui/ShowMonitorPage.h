#ifndef SHOWMONITORPAGE_H
#define SHOWMONITORPAGE_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QDateTime>
#include <QQueue>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

// 原有的头文件引用（保持不变）
#include "../capture/capturethread.h"
#include "../capture/rtspthread.h"
#include "../capture/usbcapturethread.h"
#include "../ai/aidetectionthread.h"
#include "../ai/detectionvisualizer.h"
#include "../capture/recordmanager.h"
#include "../widgets/aicontrolwidget.h"

// 性能监控相关结构（简化版本）
struct PerformanceMetrics {
    double currentFPS = 0.0;
    double avgFPS = 0.0;
    QSize resolution = QSize(0, 0);
    QString colorFormat;
    double bitrate = 0.0;        // Mbps
    int frameLatency = 0;        // ms
    double cpuUsage = 0.0;       // %
    double memoryUsage = 0.0;    // MB
    int droppedFrames = 0;
    double bufferLevel = 0.0;    // %
};

class PerformanceMonitor : public QObject
{
    Q_OBJECT

public:
    PerformanceMonitor(QObject* parent = nullptr);

    void recordFrame(const QImage& frame);
    PerformanceMetrics getMetrics() const { return m_metrics; }

signals:
    void metricsUpdated(const PerformanceMetrics& metrics);

private slots:
    void calculateFPS();
    void updateSystemMetrics();
    void debugOutputMetrics();

private:
    PerformanceMetrics m_metrics;

    QQueue<QDateTime> m_frameTimestamps;
    QTimer* m_metricsTimer;
    QDateTime m_lastFrameTime;

    qint64 m_totalFrames = 0;
    qint64 m_droppedFrames = 0;
    double m_totalDataReceived = 0.0;

    double getCPUUsage();
    double getMemoryUsage();
};

class ShowMonitorPage : public QWidget
{
    Q_OBJECT

public:
    ShowMonitorPage(const int type, const QString& rtspUrl,
                    CaptureThread* mipiThread,
                    RtspThread* rtspThread1,
                    RtspThread* rtspThread2,
                    USBCaptureThread* usbThread,
                    QWidget *parent = nullptr);
    ~ShowMonitorPage();

    void setType(int type);
    void setStreamUrl(const QString& url);

signals:
    void backToMonitorList();

public slots:
    // 原有的AI相关槽函数（保持不变）
    void onDetectionResult(const DetectionResult& result);
    void onAIToggled(bool enabled);
    void onVisualizationToggled(bool enabled);
    void onRecordingToggled(bool enabled);
    void onConfigChanged(const AIConfig& config);
    void onRecordingStateChanged(RecordManager::RecordState state);

private slots:
    // 原有的槽函数（保持不变）
    void toggleAI(bool enabled);
    void toggleAIControl();

    // 性能监控槽函数
    void onPerformanceMetricsUpdated(const PerformanceMetrics& metrics);
    void togglePerformanceDisplay();

private:
    // 原有UI组件（保持不变）
    QLabel* videoLabel;
    QLabel* streamLabel;
    QWidget* buttonContainer;
    QPushButton* backButton;
    QPushButton* aiButton;
    QPushButton* aiControlButton;

    // 原有AI相关组件（保持不变）
    DetectionVisualizer* m_visualizer;
    RecordManager* m_recordManager;
    AIControlWidget* m_aiControlWidget;
    DetectionResult m_lastDetectionResult;
    bool m_showDetectionOverlay = false;
    bool m_aiControlVisible = false;

    // 原有线程引用（保持不变）
    CaptureThread* captureThread;
    RtspThread* rtspThread1;
    RtspThread* rtspThread2;
    USBCaptureThread* usbCaptureThread;
    AIDetectionThread* aiDetectionThread;

    // 原有状态变量（保持不变）
    int m_type;
    bool aiEnabled = false;

    // 新增的性能监控组件（仅此部分是新的）
    QLabel* performanceLabel;
    QPushButton* performanceButton;
    PerformanceMonitor* m_performanceMonitor;
    bool m_showPerformanceMetrics = true;

    // 原有的私有方法（保持不变）
    void initUI();
    void initStreamLabel();
    void initVideoLabel();
    void initButtonArea();
    void initBackButton();
    void initAiButton();
    void initAiControlButton();
    void setupAIComponents();
    void initConnections();

    void connectMipiThread();
    void connectRtspThread1();
    void connectRtspThread2();
    void connectUSBThread();
    void disconnectThreads();
    void updateDisplayImage(const QImage& originalImage);

    // 新增的性能监控相关方法（仅此部分是新的）
    void initPerformanceLabel();
    void updatePerformanceDisplay(const PerformanceMetrics& metrics);
};

#endif // SHOWMONITORPAGE_H
