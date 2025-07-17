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
#include "../capture/usbcapturethread.h"
#include "../ai/aidetectionthread.h"
#include "../ai/detectionvisualizer.h"
#include "../capture/recordmanager.h"
#include "../widgets/aicontrolwidget.h"

class ShowMonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShowMonitorPage(const int type, const QString& rtspUrl,
                            CaptureThread* mipiThread,
                            RtspThread* rtspThread1,
                            RtspThread* rtspThread2,
                            USBCaptureThread* usbThread,
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
    void initAiButton();
    void initAiControlButton();
    void connectMipiThread();
    void connectRtspThread1();
    void connectRtspThread2();
    void connectUSBThread();
    void disconnectThreads();
    void initConnections();
    // ai
    void setupAIComponents();
    void updateDisplayImage(const QImage& originalImage);
    void toggleAI(bool enabled);
    
    // void initMipiCaptureThread();
    // void initRtspCaptureThread();

    // UI控件
    QLabel *streamLabel;
    QPushButton *backButton;
    QLabel *videoLabel;
    QWidget *buttonContainer;  // 新增的按钮容器成员变量
    QPushButton* aiButton;  // 新增 AI 按钮
    QPushButton* aiControlButton;

    CaptureThread *captureThread;
    RtspThread *rtspThread1;
    RtspThread *rtspThread2;
    USBCaptureThread* usbCaptureThread;
    int m_type;
    QString m_rtspUrl;


    // 新增AI相关成员
    bool aiEnabled = false; // AI 状态
    DetectionVisualizer* m_visualizer;
    RecordManager* m_recordManager;
    AIControlWidget* m_aiControlWidget;

    DetectionResult m_lastDetectionResult;
    bool m_showDetectionOverlay = true;
    bool m_aiControlVisible = false;

    AIDetectionThread* aiDetectionThread;

private slots:
    // 新增槽函数
    void onDetectionResult(const DetectionResult& result);
    void onAIToggled(bool enabled);
    void onVisualizationToggled(bool enabled);
    void onRecordingToggled(bool enabled);
    void onConfigChanged(const AIConfig& config);
    void onRecordingStateChanged(RecordManager::RecordState state);
    void toggleAIControl();
};

#endif // SHOWMONITORPAGE_H
