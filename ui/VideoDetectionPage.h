#ifndef VIDEODETECTIONPAGE_H
#define VIDEODETECTIONPAGE_H

#include <QWidget>

class VideoDetectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit VideoDetectionPage(QWidget* parent = nullptr);

signals:
    void returnRequested();

};

#endif // VIDEODETECTIONPAGE_H
#pragma once
