#ifndef SECUREVISION_H
#define SECUREVISION_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <memory>

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

private slots:  
    void handleDetectClick(int index);  // 处理检测页卡片点击
    void returnToDetectionPage();     // 返回篡改检测页
    void switchRightPage(int index);  // 切换右侧页面（监控/异常/篡改）

    void showMonitorStream(const QString& rtspUrl);  // 进入 ShowMonitorPage
    void returnToMonitorList();  // 返回 MonitorListPage

private:
    void initializeWindow();
    void setupUi();
    void createLeftNavigationBar();
    void setupGlobalStack();

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
};

#endif // SECUREVISION_H
