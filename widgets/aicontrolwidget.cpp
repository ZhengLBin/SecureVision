#include "aicontrolwidget.h"
#include <QDebug>

AIControlWidget::AIControlWidget(QWidget *parent)
    : QWidget(parent)
{
    qDebug() << "AIControlWidget: Starting initialization";

    setupUI();
    updateButtonStyles();

    // 设置窗口属性 - 适应横屏布局
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(800, 500);  // 改为横向布局尺寸

    // 初始化配置
    m_currentConfig.enableAI = false;
    m_currentConfig.enableMotionDetect = true;
    m_currentConfig.motionThreshold = 0.3f;
    m_currentConfig.skipFrames = 2;

    qDebug() << "AIControlWidget: Initialization completed";
}

void AIControlWidget::setupUI()
{
    qDebug() << "AIControlWidget: Setting up new UI design";

    // 主容器
    QWidget* container = new QWidget(this);
    container->setStyleSheet(
        "QWidget {"
        "background-color: rgba(30, 30, 30, 240);"
        "border-radius: 20px;"
        "border: 2px solid rgba(255, 255, 255, 50);"
        "}"
        );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(container);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    QVBoxLayout* containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(15);
    containerLayout->setContentsMargins(20, 20, 20, 20);

    // 标题
    QLabel* titleLabel = new QLabel("🤖 AI 智能控制", container);
    titleLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 24px;"
        "font-weight: bold;"
        "padding: 10px;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #667eea, stop:1 #764ba2);"
        "border-radius: 15px;"
        "}"
        );
    titleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(titleLabel);

    // 创建横向两列布局
    QHBoxLayout* cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(15);

    // 左列
    QVBoxLayout* leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(15);

    // 右列
    QVBoxLayout* rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(15);

    // 卡片1: AI总开关 (左列)
    createAIToggleCard(leftColumn);

    // 卡片2: 功能开关 (右列)
    createFunctionSwitchCard(rightColumn);

    // 卡片3: 参数调节 (左列)
    createParameterCard(leftColumn);

    // 卡片4: 状态显示 (右列)
    createStatusCard(rightColumn);

    // 添加伸缩项使两列高度平衡
    leftColumn->addStretch();
    rightColumn->addStretch();

    // 将两列添加到横向布局
    cardsLayout->addLayout(leftColumn);
    cardsLayout->addLayout(rightColumn);

    // 将横向布局添加到主容器
    containerLayout->addLayout(cardsLayout);
    containerLayout->addStretch();
}

void AIControlWidget::createAIToggleCard(QVBoxLayout* layout)
{
    QWidget* card = new QWidget();
    card->setStyleSheet(
        "QWidget {"
        "background-color: rgba(60, 60, 60, 180);"
        "border-radius: 15px;"
        "border: 1px solid rgba(255, 255, 255, 30);"
        "}"
        );

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 15, 20, 15);

    // 卡片标题
    QLabel* cardTitle = new QLabel("🎯 AI 检测开关");
    cardTitle->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(cardTitle);

    // AI开关按钮
    m_aiButton = new QPushButton("🔴 启动 AI 检测");
    m_aiButton->setCheckable(true);
    m_aiButton->setFixedHeight(60);
    m_aiButton->setStyleSheet(
        "QPushButton {"
        "border-radius: 30px;"
        "font-size: 18px;"
        "font-weight: bold;"
        "padding: 10px;"
        "background-color: rgba(100, 100, 100, 200);"
        "color: white;"
        "border: 2px solid rgba(150, 150, 150, 100);"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(120, 120, 120, 220);"
        "border: 2px solid rgba(180, 180, 180, 150);"
        "}"
        "QPushButton:pressed {"
        "background-color: rgba(80, 80, 80, 200);"
        "}"
        );

    bool connected = connect(m_aiButton, &QPushButton::toggled, this, &AIControlWidget::onAIToggled);
    qDebug() << "AIControlWidget: AI button signal connected:" << connected;

    cardLayout->addWidget(m_aiButton);
    layout->addWidget(card);
}

void AIControlWidget::createFunctionSwitchCard(QVBoxLayout* layout)
{
    QWidget* card = new QWidget();
    card->setStyleSheet(
        "QWidget {"
        "background-color: rgba(60, 60, 60, 180);"
        "border-radius: 15px;"
        "border: 1px solid rgba(255, 255, 255, 30);"
        "}"
        );

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 15, 20, 15);
    cardLayout->setSpacing(15);

    // 卡片标题
    QLabel* cardTitle = new QLabel("⚙️ 功能开关");
    cardTitle->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(cardTitle);

    // 可视化开关
    m_visualizationCheckBox = new QCheckBox("🎨 显示检测框");
    m_visualizationCheckBox->setChecked(true);
    m_visualizationCheckBox->setStyleSheet(
        "QCheckBox {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 8px;"
        "spacing: 10px;"
        "}"
        "QCheckBox::indicator {"
        "width: 25px;"
        "height: 25px;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "background-color: rgba(100, 100, 100, 150);"
        "border: 3px solid rgba(200, 200, 200, 100);"
        "border-radius: 5px;"
        "}"
        "QCheckBox::indicator:checked {"
        "background-color: #4CAF50;"
        "border: 3px solid #4CAF50;"
        "border-radius: 5px;"
        "image: url(data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMTYiIGhlaWdodD0iMTYiIHZpZXdCb3g9IjAgMCAxNiAxNiIgZmlsbD0ibm9uZSIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj4KPHBhdGggZD0iTTEzLjMzMzMgNC42NjY2N0w2IDEyTDIuNjY2NjcgOC42NjY2NyIgc3Ryb2tlPSJ3aGl0ZSIgc3Ryb2tlLXdpZHRoPSIyIiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiLz4KPC9zdmc+);"
        "}"
        "QCheckBox::indicator:hover {"
        "border: 3px solid rgba(220, 220, 220, 200);"
        "}"
        );

    bool visConnected = connect(m_visualizationCheckBox, &QCheckBox::toggled,
                                this, &AIControlWidget::onVisualizationToggled);
    qDebug() << "AIControlWidget: Visualization checkbox signal connected:" << visConnected;

    cardLayout->addWidget(m_visualizationCheckBox);

    // 录制开关
    m_recordingCheckBox = new QCheckBox("📹 启用智能录制");
    m_recordingCheckBox->setChecked(true);
    m_recordingCheckBox->setStyleSheet(m_visualizationCheckBox->styleSheet());

    bool recConnected = connect(m_recordingCheckBox, &QCheckBox::toggled,
                                this, &AIControlWidget::onRecordingToggled);
    qDebug() << "AIControlWidget: Recording checkbox signal connected:" << recConnected;

    cardLayout->addWidget(m_recordingCheckBox);
    layout->addWidget(card);
}

void AIControlWidget::createParameterCard(QVBoxLayout* layout)
{
    QWidget* card = new QWidget();
    card->setStyleSheet(
        "QWidget {"
        "background-color: rgba(60, 60, 60, 180);"
        "border-radius: 15px;"
        "border: 1px solid rgba(255, 255, 255, 30);"
        "}"
        );

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 15, 20, 15);
    cardLayout->setSpacing(15);

    // 卡片标题
    QLabel* cardTitle = new QLabel("🎛️ 检测参数");
    cardTitle->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(cardTitle);

    // 灵敏度标签
    QLabel* sensitivityLabel = new QLabel("🎯 检测灵敏度");
    sensitivityLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 14px;"
        "font-weight: bold;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(sensitivityLabel);

    // 灵敏度滑块容器
    QWidget* sliderContainer = new QWidget();
    QHBoxLayout* sliderLayout = new QHBoxLayout(sliderContainer);
    sliderLayout->setContentsMargins(0, 0, 0, 0);

    // 滑块
    m_thresholdSlider = new QSlider(Qt::Horizontal);
    m_thresholdSlider->setRange(10, 90);
    m_thresholdSlider->setValue(30);
    m_thresholdSlider->setFixedHeight(30);
    m_thresholdSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "border: none;"
        "height: 10px;"
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FF6B6B, stop:0.5 #4ECDC4, stop:1 #45B7D1);"
        "border-radius: 5px;"
        "}"
        "QSlider::handle:horizontal {"
        "background: white;"
        "border: 3px solid #333;"
        "width: 24px;"
        "height: 24px;"
        "border-radius: 12px;"
        "margin: -7px 0;"
        "}"
        "QSlider::handle:horizontal:hover {"
        "background: #f0f0f0;"
        "border: 3px solid #555;"
        "}"
        "QSlider::handle:horizontal:pressed {"
        "background: #e0e0e0;"
        "border: 3px solid #777;"
        "}"
        );

    // 数值标签
    m_thresholdLabel = new QLabel("30%");
    m_thresholdLabel->setStyleSheet(
        "QLabel {"
        "color: #4ECDC4;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px;"
        "border: 2px solid #4ECDC4;"
        "border-radius: 8px;"
        "background-color: rgba(78, 205, 196, 20);"
        "}"
        );
    m_thresholdLabel->setFixedWidth(60);
    m_thresholdLabel->setAlignment(Qt::AlignCenter);

    bool sliderConnected = connect(m_thresholdSlider, &QSlider::valueChanged,
                                   this, &AIControlWidget::onThresholdChanged);
    qDebug() << "AIControlWidget: Threshold slider signal connected:" << sliderConnected;

    sliderLayout->addWidget(m_thresholdSlider);
    sliderLayout->addWidget(m_thresholdLabel);

    cardLayout->addWidget(sliderContainer);

    // 灵敏度说明
    QLabel* sensitivityHint = new QLabel("💡 数值越小越敏感");
    sensitivityHint->setStyleSheet(
        "QLabel {"
        "color: rgba(255, 255, 255, 150);"
        "font-size: 12px;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(sensitivityHint);

    layout->addWidget(card);
}

void AIControlWidget::createStatusCard(QVBoxLayout* layout)
{
    QWidget* card = new QWidget();
    card->setStyleSheet(
        "QWidget {"
        "background-color: rgba(60, 60, 60, 180);"
        "border-radius: 15px;"
        "border: 1px solid rgba(255, 255, 255, 30);"
        "}"
        );

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 15, 20, 15);
    cardLayout->setSpacing(10);

    // 卡片标题
    QLabel* cardTitle = new QLabel("📊 实时状态");
    cardTitle->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 16px;"
        "font-weight: bold;"
        "padding: 5px;"
        "}"
        );
    cardLayout->addWidget(cardTitle);

    // 检测状态
    m_statusLabel = new QLabel("🔴 AI检测未启动");
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "color: white;"
        "font-size: 14px;"
        "font-weight: bold;"
        "padding: 12px;"
        "background-color: rgba(0, 0, 0, 100);"
        "border-radius: 8px;"
        "border: 1px solid rgba(255, 255, 255, 50);"
        "}"
        );
    cardLayout->addWidget(m_statusLabel);

    // 录制状态
    m_recordStatusLabel = new QLabel("📹 状态: 待机");
    m_recordStatusLabel->setStyleSheet(
        "QLabel {"
        "color: #FFA500;"
        "font-size: 14px;"
        "font-weight: bold;"
        "padding: 12px;"
        "background-color: rgba(0, 0, 0, 100);"
        "border-radius: 8px;"
        "border: 1px solid rgba(255, 165, 0, 50);"
        "}"
        );
    cardLayout->addWidget(m_recordStatusLabel);

    layout->addWidget(card);
}

// =====================================================
// 槽函数实现
// =====================================================

void AIControlWidget::onAIToggled(bool enabled)
{
    qDebug() << "AIControlWidget::onAIToggled called with enabled =" << enabled;

    m_aiEnabled = enabled;
    m_currentConfig.enableAI = enabled;

    updateButtonStyles();
    updateStatusDisplay();

    qDebug() << "AIControlWidget: Emitting aiToggled signal";
    emit aiToggled(enabled);
    emit configChanged(m_currentConfig);
}

void AIControlWidget::onVisualizationToggled(bool enabled)
{
    qDebug() << "AIControlWidget::onVisualizationToggled called with enabled =" << enabled;

    m_visualizationEnabled = enabled;

    qDebug() << "AIControlWidget: Emitting visualizationToggled signal";
    emit visualizationToggled(enabled);
}

void AIControlWidget::onRecordingToggled(bool enabled)
{
    qDebug() << "AIControlWidget::onRecordingToggled called with enabled =" << enabled;

    m_recordingEnabled = enabled;

    qDebug() << "AIControlWidget: Emitting recordingToggled signal";
    emit recordingToggled(enabled);
}

void AIControlWidget::onThresholdChanged(int value)
{
    qDebug() << "AIControlWidget::onThresholdChanged called with value =" << value;

    float threshold = value / 100.0f;
    m_currentConfig.motionThreshold = threshold;

    m_thresholdLabel->setText(QString("%1%").arg(value));

    // 根据阈值改变标签颜色
    QString color;
    QString bgColor;
    if (value < 30) {
        color = "#FF6B6B";
        bgColor = "rgba(255, 107, 107, 20)";
    } else if (value < 60) {
        color = "#4ECDC4";
        bgColor = "rgba(78, 205, 196, 20)";
    } else {
        color = "#45B7D1";
        bgColor = "rgba(69, 183, 209, 20)";
    }

    m_thresholdLabel->setStyleSheet(
        QString("QLabel {"
                "color: %1;"
                "font-size: 16px;"
                "font-weight: bold;"
                "padding: 5px;"
                "border: 2px solid %1;"
                "border-radius: 8px;"
                "background-color: %2;"
                "}").arg(color).arg(bgColor)
        );

    qDebug() << "AIControlWidget: Emitting configChanged signal";
    emit configChanged(m_currentConfig);
}

void AIControlWidget::updateDetectionStatus(const DetectionResult& result)
{
    if (!m_aiEnabled) return;

    QString statusText;
    QString statusColor;
    QString bgColor;

    if (result.hasMotion) {
        statusText = QString("🟢 检测到运动 [%1×%2]")
                         .arg(result.motionArea.width())
                         .arg(result.motionArea.height());
        statusColor = "#4CAF50";
        bgColor = "rgba(76, 175, 80, 20)";
    } else {
        statusText = "⚪ 监控中...";
        statusColor = "#888888";
        bgColor = "rgba(136, 136, 136, 20)";
    }

    if (!result.faces.isEmpty()) {
        statusText += QString(" | 👤 %1张人脸").arg(result.faces.size());
    }

    m_statusLabel->setText(statusText);
    m_statusLabel->setStyleSheet(
        QString("QLabel {"
                "color: %1;"
                "font-size: 14px;"
                "font-weight: bold;"
                "padding: 12px;"
                "background-color: %2;"
                "border-radius: 8px;"
                "border: 1px solid %1;"
                "}").arg(statusColor).arg(bgColor)
        );
}

void AIControlWidget::updateRecordingStatus(RecordManager::RecordState state)
{
    QString statusText;
    QString statusColor;
    QString bgColor;

    switch (state) {
    case RecordManager::Idle:
        statusText = "📹 状态: 待机";
        statusColor = "#888888";
        bgColor = "rgba(136, 136, 136, 20)";
        break;
    case RecordManager::PreRecord:
        statusText = "📹 状态: 准备录制...";
        statusColor = "#FFA500";
        bgColor = "rgba(255, 165, 0, 20)";
        break;
    case RecordManager::Recording:
        statusText = "📹 状态: 🔴 正在录制";
        statusColor = "#FF4444";
        bgColor = "rgba(255, 68, 68, 20)";
        break;
    case RecordManager::PostRecord:
        statusText = "📹 状态: 延迟录制...";
        statusColor = "#FF8800";
        bgColor = "rgba(255, 136, 0, 20)";
        break;
    case RecordManager::Cooldown:
        statusText = "📹 状态: 冷却中...";
        statusColor = "#4CAF50";
        bgColor = "rgba(76, 175, 80, 20)";
        break;
    }

    m_recordStatusLabel->setText(statusText);
    m_recordStatusLabel->setStyleSheet(
        QString("QLabel {"
                "color: %1;"
                "font-size: 14px;"
                "font-weight: bold;"
                "padding: 12px;"
                "background-color: %2;"
                "border-radius: 8px;"
                "border: 1px solid %1;"
                "}").arg(statusColor).arg(bgColor)
        );
}

void AIControlWidget::updateButtonStyles()
{
    if (m_aiEnabled) {
        m_aiButton->setText("🟢 AI检测运行中");
        m_aiButton->setStyleSheet(
            "QPushButton {"
            "border-radius: 30px;"
            "font-size: 18px;"
            "font-weight: bold;"
            "padding: 10px;"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #45a049);"
            "color: white;"
            "border: 2px solid #4CAF50;"
            "}"
            "QPushButton:hover {"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #5CBF60, stop:1 #55b059);"
            "}"
            "QPushButton:pressed {"
            "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3e8e41, stop:1 #367c39);"
            "}"
            );
    } else {
        m_aiButton->setText("🔴 启动 AI 检测");
        m_aiButton->setStyleSheet(
            "QPushButton {"
            "border-radius: 30px;"
            "font-size: 18px;"
            "font-weight: bold;"
            "padding: 10px;"
            "background-color: rgba(100, 100, 100, 200);"
            "color: white;"
            "border: 2px solid rgba(150, 150, 150, 100);"
            "}"
            "QPushButton:hover {"
            "background-color: rgba(120, 120, 120, 220);"
            "border: 2px solid rgba(180, 180, 180, 150);"
            "}"
            "QPushButton:pressed {"
            "background-color: rgba(80, 80, 80, 200);"
            "}"
            );
    }
}

void AIControlWidget::updateStatusDisplay()
{
    if (m_aiEnabled) {
        m_statusLabel->setText("🟢 AI检测已启动，等待画面...");
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "color: #4CAF50;"
            "font-size: 14px;"
            "font-weight: bold;"
            "padding: 12px;"
            "background-color: rgba(76, 175, 80, 20);"
            "border-radius: 8px;"
            "border: 1px solid #4CAF50;"
            "}"
            );
    } else {
        m_statusLabel->setText("🔴 AI检测未启动");
        m_statusLabel->setStyleSheet(
            "QLabel {"
            "color: #888888;"
            "font-size: 14px;"
            "font-weight: bold;"
            "padding: 12px;"
            "background-color: rgba(136, 136, 136, 20);"
            "border-radius: 8px;"
            "border: 1px solid #888888;"
            "}"
            );
    }
}
