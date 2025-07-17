#include "motiondetector.h"
#include <QDebug>

MotionDetector::MotionDetector(QObject *parent)
    : QObject(parent)
{
}

bool MotionDetector::detectMotion(const QImage& currentFrame, QRect& motionArea)
{
    if (currentFrame.isNull()) {
        qDebug() << "MotionDetector: Received null image";
        return false;
    }

    cv::Mat frame = qImageToCvMat(currentFrame);
    if (frame.empty()) {
        qDebug() << "MotionDetector: Failed to convert QImage to cv::Mat";
        return false;
    }

    // 转换为灰度图
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // 高斯模糊
    cv::GaussianBlur(gray, gray, cv::Size(21, 21), 0);

    // 初始化背景模型
    if (!m_initialized) {
        gray.copyTo(m_background);
        m_initialized = true;
        qDebug() << "MotionDetector: Background model initialized";
        return false;
    }

    // 计算帧差
    cv::Mat diff;
    cv::absdiff(m_background, gray, diff);

    // 二值化
    cv::Mat thresh;
    cv::threshold(diff, thresh, 25, 255, cv::THRESH_BINARY);

    // 形态学操作
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(thresh, thresh, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(thresh, thresh, cv::MORPH_CLOSE, kernel);

    // 查找轮廓
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // 查找最大轮廓
    bool hasMotion = false;
    cv::Rect maxRect;
    double maxArea = 0;
    int validContours = 0;

    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        if (area > 500) {  // 最小面积阈值
            validContours++;
            cv::Rect rect = cv::boundingRect(contour);
            if (area > maxArea) {
                maxArea = area;
                maxRect = rect;
                hasMotion = true;
            }
        }
    }

    if (hasMotion) {
        motionArea = cvRectToQRect(maxRect);
        qDebug() << "MotionDetector: Motion detected! Area:" << maxArea
                 << "Valid contours:" << validContours
                 << "Motion rect:" << motionArea;
    }

    // 更新背景模型 (简单的学习率)
    cv::addWeighted(m_background, 0.95, gray, 0.05, 0, m_background);

    return hasMotion;
}

void MotionDetector::reset()
{
    m_initialized = false;
    m_background.release();
}

cv::Mat MotionDetector::qImageToCvMat(const QImage& qImage)
{
    QImage image = qImage;

    // 确保图像格式为RGB888
    if (image.format() != QImage::Format_RGB888) {
        image = image.convertToFormat(QImage::Format_RGB888);
    }

    // 交换RGB到BGR（OpenCV使用BGR）
    QImage swapped = image.rgbSwapped();

    return cv::Mat(swapped.height(), swapped.width(), CV_8UC3,
                   (void*)swapped.constBits(), swapped.bytesPerLine()).clone();
}

QRect MotionDetector::cvRectToQRect(const cv::Rect& cvRect)
{
    return QRect(cvRect.x, cvRect.y, cvRect.width, cvRect.height);
}
