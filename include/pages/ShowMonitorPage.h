#ifndef SHOWMONITORPAGE_H
#define SHOWMONITORPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class ShowMonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit ShowMonitorPage(QWidget *parent = nullptr);
    void setStreamUrl(const QString& url);  // 用于接收 RTSP 地址

signals:
    void backToMonitorList();  // 返回按钮信号

private:
    QLabel* titleLabel;
    QLabel* streamLabel;  // 用于显示 RTSP 地址
    QPushButton* backButton;
};

#endif // SHOWMONITORPAGE_H
