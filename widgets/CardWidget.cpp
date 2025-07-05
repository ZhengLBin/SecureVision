#include "CardWidget.h"
//#include "secureVision.h"
#include <QEvent>
#include <QMouseEvent>
#include <QPropertyAnimation>

CardWidget::CardWidget(const QString& iconPath, const QString& title, const QString& description, QWidget* parent)
    : QWidget(parent)
{
    this->setFixedSize(450, 300);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 创建一个容器用于放置三个 QLabel
    QWidget* labelContainer = new QWidget(this);
    labelContainer->setStyleSheet("border-radius: 10px; border: 2px solid qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 red, stop:1 blue);");

    QVBoxLayout* labelLayout = new QVBoxLayout(labelContainer);
    labelLayout->setSpacing(20);
    labelLayout->setContentsMargins(10, 10, 10, 10);

    // 使用 QHBoxLayout 将 logoLabel 和 titleLabel 放在同一行
    QHBoxLayout* logoTitleLayout = new QHBoxLayout();
    logoTitleLayout->setSpacing(20); // 设置 logo 和 title 之间的间距
    logoTitleLayout->setContentsMargins(0, 0, 0, 0); // 设置边距为 0

    logoTitleLayout->addStretch();

    QLabel* logoLabel = new QLabel(labelContainer);
    logoLabel->setPixmap(QPixmap(iconPath).scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setStyleSheet("background: transparent; border: none;");
    logoTitleLayout->addWidget(logoLabel, 0, Qt::AlignLeft);

    QLabel* titleLabel = new QLabel(title, labelContainer);
    titleLabel->setStyleSheet("font-size: 50px; font-weight: bold;color: #333; background: transparent; border: none;");
    logoTitleLayout->addWidget(titleLabel, 0, Qt::AlignLeft);

    // 添加一个伸缩项，确保 logo 和 title 整体靠左
    logoTitleLayout->addStretch();

    // 将 logoTitleLayout 添加到 labelLayout 中
    labelLayout->addLayout(logoTitleLayout);

    QLabel* descriptionLabel = new QLabel(description, labelContainer);
    descriptionLabel->setStyleSheet("font-size: 30px; color: #666; background: transparent; border: none;");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setFixedWidth(360);
    descriptionLabel->setAlignment(Qt::AlignHCenter); // 设置 descriptionLabel 水平居中
    labelLayout->addWidget(descriptionLabel, 0, Qt::AlignHCenter);

    mainLayout->addWidget(labelContainer);
    setLayout(mainLayout);

    //connect(this, &CardWidget::cardClicked, dynamic_cast<SecureVision*>(parent), &SecureVision::handleCardClick);
}


// 重写鼠标事件触发信号
void CardWidget::mousePressEvent(QMouseEvent* event)
{
    int cardId = 10000;  // 假设我们发送一个 ID，你可以根据实际需求设置
    emit cardClicked(cardId);  // 发出带有参数的信号
    QWidget::mousePressEvent(event); // 调用父类的鼠标事件处理
}
