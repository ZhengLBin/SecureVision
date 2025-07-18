#ifndef DETECTIONVISUALIZER_H
#define DETECTIONVISUALIZER_H

#include <QObject>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include "aitypes.h"

class DetectionVisualizer : public QObject
{
    Q_OBJECT

public:
    explicit DetectionVisualizer(QObject *parent = nullptr);

    // 主要绘制接口
    QImage drawDetections(const QImage& originalImage, const DetectionResult& result);

    // 配置接口
    void setMotionBoxColor(const QColor& color) { m_motionBoxColor = color; }
    // void setFaceBoxColor(const QColor& color) { m_faceBoxColor = color; }
    void setShowTimestamp(bool show) { m_showTimestamp = show; }
    void setShowConfidence(bool show) { m_showConfidence = show; }
    void setShowStatusInfo(bool show) { m_showStatusInfo = show; }

    void drawFaceDetections(QPainter& painter, const QVector<FaceInfo>& faces, const QSize& imageSize);
    void setFaceBoxColor(const QColor& color) { m_faceBoxColor = color; }
    void setRecognizedFaceColor(const QColor& color) { m_recognizedFaceColor = color; }
    void setUnknownFaceColor(const QColor& color) { m_unknownFaceColor = color; }

private:
    // 可视化配置
    QColor m_motionBoxColor = QColor(0, 255, 0);      // 绿色运动框
    //QColor m_faceBoxColor = QColor(255, 255, 0);      // 黄色人脸框
    QColor m_textColor = QColor(255, 255, 255);       // 白色文字
    QColor m_backgroundColor = QColor(0, 0, 0, 150);  // 半透明黑色背景

    bool m_showTimestamp = true;
    bool m_showConfidence = true;
    bool m_showStatusInfo = true;
    int m_boxLineWidth = 3;
    int m_fontSize = 16;

    // 绘制辅助函数
    void drawMotionBox(QPainter& painter, const QRect& motionArea);
    void drawFaceBoxes(QPainter& painter, const QVector<QRect>& faces,
                       const QVector<float>& confidences);
    void drawStatusInfo(QPainter& painter, const DetectionResult& result,
                        const QSize& imageSize);
    void drawTimestamp(QPainter& painter, const QDateTime& timestamp,
                       const QSize& imageSize);

    // 🆕 人脸可视化相关
    QColor m_faceBoxColor = QColor(0, 255, 0);        // 绿色：一般人脸框
    QColor m_recognizedFaceColor = QColor(0, 255, 0);  // 绿色：已识别人脸
    QColor m_unknownFaceColor = QColor(255, 165, 0);   // 橙色：未知人脸

    // 🆕 人脸绘制方法
    void drawSingleFace(QPainter& painter, const FaceInfo& face);
    void drawFaceLabel(QPainter& painter, const QRect& faceRect, const QString& label, const QColor& color);
    QRect calculateLabelRect(const QRect& faceRect, const QString& label, const QFont& font);
};

#endif // DETECTIONVISUALIZER_H
