#include "../../include/pages/ShowMonitorPage.h"
#include <QVBoxLayout>

ShowMonitorPage::ShowMonitorPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    titleLabel = new QLabel("Monitor Stream", this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; text-align: center;");
    
    streamLabel = new QLabel("RTSP Stream URL: ", this);
    streamLabel->setStyleSheet("font-size: 18px; color: #555;");

    backButton = new QPushButton("Back", this);
    backButton->setFixedSize(100, 40);
    backButton->setStyleSheet("background-color: #007BFF; color: white; font-size: 16px;");

    connect(backButton, &QPushButton::clicked, this, &ShowMonitorPage::backToMonitorList);

    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(streamLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(backButton, 0, Qt::AlignCenter);
    
    setLayout(mainLayout);
}

void ShowMonitorPage::setStreamUrl(const QString& url)
{
    streamLabel->setText("RTSP Stream URL: " + url);
}
