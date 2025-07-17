#ifndef MOTIONDETECTOR_H
#define MOTIONDETECTOR_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <opencv2/opencv.hpp>

class MotionDetector : public QObject
{
    Q_OBJECT

public:
    explicit MotionDetector(QObject *parent = nullptr);

    // 设置参数
    void setThreshold(float threshold) { m_threshold = threshold; }
    void setROI(const QRect& roi) { m_roiArea = roi; }

    // 检测移动
    bool detectMotion(const QImage& currentFrame, QRect& motionArea);

    // 重置背景模型
    void reset();

private:
    cv::Mat m_background;           // 背景模型
    cv::Mat m_previousFrame;        // 上一帧
    float m_threshold = 0.3f;       // 检测阈值
    QRect m_roiArea;               // 检测区域
    bool m_initialized = false;     // 是否已初始化

    // 辅助函数
    cv::Mat qImageToCvMat(const QImage& qImage);
    QRect cvRectToQRect(const cv::Rect& cvRect);
};

#endif // MOTIONDETECTOR_H
