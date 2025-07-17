#ifndef SECUREVISION_H
#define SECUREVISION_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <memory>

#include "../capture/capturethread.h"
#include "../capture/rtspthread.h"
#include "../capture/usbcapturethread.h"
#include "../ai/aidetectionthread.h"
#include "../ai/aitypes.h"

class MonitorListPage;
class DetectionListPage;
class VideoDetectionPage;
class ShowMonitorPage;
class AudioDetectionPage;

class SecureVision : public QMainWindow
{
    Q_OBJECT

public:
    explicit SecureVision(QWidget* parent = nullptr);
    ~SecureVision() override;

    AIDetectionThread* getAIThread() const { return aiThread; }

private slots:  
    void handleDetectClick(int index);  // 处理检测页卡片点击
    void returnToDetectionPage();     // 返回篡改检测页
    void switchRightPage(int index);  // 切换右侧页面（监控/异常/篡改）

    void showMonitorStream(const QString& rtspUrl, const int m_type);  // 进入 ShowMonitorPage
    void returnToMonitorList();  // 返回 MonitorListPage

    // 新增AI相关槽函数
    void onDetectionResult(const DetectionResult& result);
    void onRecordTrigger(RecordTrigger trigger, const QImage& frame);

private:
    void initializeWindow();
    void setupUi();
    void createLeftNavigationBar();
    void setupGlobalStack();
    void setupThread();

    CaptureThread* mipiThread = nullptr;
    RtspThread* rtspThread1 = nullptr;
    RtspThread* rtspThread2 = nullptr;
    USBCaptureThread* usbThread = nullptr;

    // UI Components
    QStackedWidget* globalStack;     // 全局堆栈：管理两种模式
    QWidget* normalModeWidget;       // 模式1：左侧导航栏 + rightStack
    QStackedWidget* rightStack;      // 右侧堆栈（监控/异常/篡改检测）
    QListWidget* leftBar;            // 左侧导航栏

    // Pages
    std::unique_ptr<MonitorListPage> monitorPage;
    std::unique_ptr<ShowMonitorPage> showMonitorPage;
    std::unique_ptr<QWidget> anomalyPage;
    std::unique_ptr<DetectionListPage> detectionPage;
    std::unique_ptr<VideoDetectionPage> videoPage;
    std::unique_ptr<AudioDetectionPage> audioPage;

    // 新增AI相关成员
    AIDetectionThread* aiThread;
    AIConfig aiConfig;

    // 新增方法声明
    void setupAIThread();
    void connectAISignals();
};

#endif // SECUREVISION_H
