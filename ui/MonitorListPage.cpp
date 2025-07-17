#include "MonitorListPage.h"


#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

const QString MonitorListPage::cardLogoPath = ":/AVDeepfake/MonitorLogo";
MonitorListPage::MonitorListPage(QWidget* parent)
    : QWidget(parent)
{
    // 初始化设备（实际项目中可以从配置文件或数据库加载）
    m_deviceManager.addDevice("MIPI 01");

    m_deviceManager.addDevice("IP 01",
                              QUrl("rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/101?transportmode=unicast"));

    m_deviceManager.addDevice("IP 02",
                              QUrl("rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/201?transportmode=unicast"));

    m_deviceManager.addUSBDevice("USB", "/dev/video45");
    setupCards();
}

void MonitorListPage::setupCards()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    QVector<Device> devices = m_deviceManager.devices();

    const int cardsPerRow = 2;
    int totalCards = devices.size();
    int rowCount = (totalCards + cardsPerRow - 1) / cardsPerRow; // 计算需要的行数

    for (int row = 0; row < rowCount; ++row) {
        QHBoxLayout* rowLayout = new QHBoxLayout();
        rowLayout->setSpacing(40); // 设置卡片之间的间距
        rowLayout->setAlignment(Qt::AlignLeft);
        
        // 计算当前行的起始和结束索引
        int startIdx = row * cardsPerRow;
        int endIdx = qMin(startIdx + cardsPerRow, totalCards);


        // 添加当前行的卡片
        for (int i = startIdx; i < endIdx; ++i) {
            int typeNum = static_cast<int>(devices[i].type());
            MonitorWidget* card = new MonitorWidget(
                cardLogoPath,
                devices[i].name(),
                typeNum,
                this
            );
            connect(card, &MonitorWidget::monitorClicked, this, &MonitorListPage::openShowMonitorPage);
            rowLayout->addWidget(card);
        }
        
        mainLayout->addLayout(rowLayout);
    }
    
    // 添加 "Add New Device" 按钮
    QPushButton* addButton = new QPushButton("Add New Device", this);
    addButton->setFixedHeight(60);  // 设置按钮高度
    addButton->setFixedWidth(250);  // 设置按钮高度
    addButton->setStyleSheet(
        "QPushButton {"
        "   background-color: #155aed;"  // 绿色背景
        "   color: white;"               // 白色文字
        "   border-radius: 5px;"         // 圆角
        "   font-size: 18px;"            // 字体大小
        "}"
        "QPushButton:hover {"
        "   background-color: #45a049;"  // 悬停时变深绿色
        "}"
    );

    // 连接按钮的点击信号（可选）
    connect(addButton, &QPushButton::clicked, this, []() {
        qDebug() << "Add New Device button clicked!";
        // 可以在这里添加设备逻辑
    });

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 20, 0);  // 右边距 20px
    buttonLayout->addStretch();  // 左侧弹性空间，使按钮右对齐
    buttonLayout->addWidget(addButton);  // 添加按钮（默认右对齐）

    // 将按钮布局添加到主布局
    mainLayout->addStretch();  // 卡片和按钮之间的弹性空间
    mainLayout->addLayout(buttonLayout);  // 添加按钮布局
    mainLayout->addSpacing(10);  // 底部间距（可选）

    setLayout(mainLayout);
}
