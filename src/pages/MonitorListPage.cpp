#include "../../include/pages/MonitorListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

MonitorListPage::MonitorListPage(QWidget* parent)
    : QWidget(parent)
{
    m_rtspStreams = {
        "mipi01"
        //"rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/101?transportmode=unicast",
        //"rtsp://admin:haikang123@192.168.16.240:554/Streaming/Channels/201?transportmode=unicast"
        // 可以继续添加更多地址
    };
    setupCards();
}

void MonitorListPage::setupCards()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(10);

    const QVector<QPair<QString, QString>> cardData = {
        {":/AVDeepfake/MonitorLogo", m_rtspStreams[0]},
    };

    const int cardsPerRow = 2;
    int totalCards = m_rtspStreams.size(); 
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
            MonitorWidget* card = new MonitorWidget(
                cardData[i].first, 
                cardData[i].second, 
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
