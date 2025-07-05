#include "DetectionListPage.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

DetectionListPage::DetectionListPage(QWidget* parent)
    : QWidget(parent)
{
    setupCards();
}

void DetectionListPage::setupCards()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(40);

    const QVector<QPair<QString, QString>> cardData = {
        {":/AVDeepfake/ViCheck", u8"视频检测"},
        {":/AVDeepfake/AuDetection", u8"音频检测"}
    };

    const QStringList descriptions = {
        u8"对视频内容进行检测处理。",
        u8"检测音频中的篡改痕迹。"
    };

    QHBoxLayout* rowLayout = new QHBoxLayout();
    
    for (int i = 0; i < cardData.size(); ++i) {
        CardWidget* card = new CardWidget(
            cardData[i].first, 
            cardData[i].second, 
            descriptions[i], 
            this
        );
        
        connect(card, &CardWidget::cardClicked, this, [this, i]() {
            emit cardClicked(i);
        });
        
        rowLayout->addWidget(card);
    }
    
    layout->addLayout(rowLayout);
    layout->addStretch();
    setLayout(layout);
}
