#include "VideoDetectionPage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QPushButton>

VideoDetectionPage::VideoDetectionPage(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* label = new QLabel(u8"这是视频检测页面", this);
    layout->addWidget(label);

    QPushButton* backButton = new QPushButton(u8"返回篡改检测页", this);
    layout->addWidget(backButton);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        emit returnRequested();  // 请求返回
    });
}
