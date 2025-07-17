#include "detectionvisualizer.h"
#include <QDebug>

DetectionVisualizer::DetectionVisualizer(QObject *parent)
    : QObject(parent)
{
    qDebug() << "DetectionVisualizer initialized";
}

QImage DetectionVisualizer::drawDetections(const QImage& originalImage, const DetectionResult& result)
{
    if (originalImage.isNull()) {
        qDebug() << "DetectionVisualizer: Received null image";
        return QImage();
    }

    // 复制原图像用于绘制
    QImage resultImage = originalImage.copy();

    QPainter painter(&resultImage);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制运动检测框
    if (result.hasMotion && !result.motionArea.isEmpty()) {
        drawMotionBox(painter, result.motionArea);
    }

    // 绘制人脸检测框
    if (!result.faces.isEmpty()) {
        drawFaceBoxes(painter, result.faces, result.confidences);
    }

    // 绘制状态信息
    if (m_showStatusInfo) {
        drawStatusInfo(painter, result, resultImage.size());
    }

    // 绘制时间戳
    if (m_showTimestamp) {
        drawTimestamp(painter, result.timestamp, resultImage.size());
    }

    return resultImage;
}

void DetectionVisualizer::drawMotionBox(QPainter& painter, const QRect& motionArea)
{
    // 设置画笔
    QPen pen(m_motionBoxColor, m_boxLineWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 绘制矩形框
    painter.drawRect(motionArea);

    // 绘制四个角的装饰
    int cornerSize = 20;
    QPen cornerPen(m_motionBoxColor, m_boxLineWidth + 1);
    painter.setPen(cornerPen);

    // 左上角
    painter.drawLine(motionArea.topLeft(),
                     motionArea.topLeft() + QPoint(cornerSize, 0));
    painter.drawLine(motionArea.topLeft(),
                     motionArea.topLeft() + QPoint(0, cornerSize));

    // 右上角
    painter.drawLine(motionArea.topRight(),
                     motionArea.topRight() + QPoint(-cornerSize, 0));
    painter.drawLine(motionArea.topRight(),
                     motionArea.topRight() + QPoint(0, cornerSize));

    // 左下角
    painter.drawLine(motionArea.bottomLeft(),
                     motionArea.bottomLeft() + QPoint(cornerSize, 0));
    painter.drawLine(motionArea.bottomLeft(),
                     motionArea.bottomLeft() + QPoint(0, -cornerSize));

    // 右下角
    painter.drawLine(motionArea.bottomRight(),
                     motionArea.bottomRight() + QPoint(-cornerSize, 0));
    painter.drawLine(motionArea.bottomRight(),
                     motionArea.bottomRight() + QPoint(0, -cornerSize));

    // 绘制标签
    QString label = "MOTION DETECTED";
    QFont font = painter.font();
    font.setPixelSize(m_fontSize);
    font.setBold(true);
    painter.setFont(font);

    // 文字背景
    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(label);
    textRect.moveTopLeft(motionArea.topLeft() + QPoint(5, -textRect.height() - 8));
    textRect.adjust(-8, -4, 8, 4);

    painter.fillRect(textRect, m_backgroundColor);
    painter.setPen(QPen(m_textColor));
    painter.drawText(textRect, Qt::AlignCenter, label);
}

void DetectionVisualizer::drawFaceBoxes(QPainter& painter, const QVector<QRect>& faces,
                                        const QVector<float>& confidences)
{
    QPen pen(m_faceBoxColor, m_boxLineWidth);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    QFont font = painter.font();
    font.setPixelSize(m_fontSize);
    font.setBold(true);
    painter.setFont(font);

    for (int i = 0; i < faces.size(); ++i) {
        const QRect& face = faces[i];

        // 绘制人脸框
        painter.drawRect(face);

        // 绘制置信度标签
        QString label = QString("FACE %1").arg(i + 1);
        if (i < confidences.size() && m_showConfidence) {
            label += QString(" (%.1f%%)").arg(confidences[i] * 100);
        }

        QFontMetrics fm(font);
        QRect textRect = fm.boundingRect(label);
        textRect.moveTopLeft(face.topLeft() + QPoint(5, -textRect.height() - 8));
        textRect.adjust(-8, -4, 8, 4);

        painter.fillRect(textRect, m_backgroundColor);
        painter.setPen(QPen(m_textColor));
        painter.drawText(textRect, Qt::AlignCenter, label);
        painter.setPen(pen); // 恢复框线颜色
    }
}

void DetectionVisualizer::drawStatusInfo(QPainter& painter, const DetectionResult& result,
                                         const QSize& imageSize)
{
    // 右上角状态信息
    QStringList statusLines;

    if (result.hasMotion) {
        statusLines << "🟢 MOTION ACTIVE";
        statusLines << QString("Size: %1×%2").arg(result.motionArea.width()).arg(result.motionArea.height());
        statusLines << QString("Pos: (%1,%2)").arg(result.motionArea.x()).arg(result.motionArea.y());
    } else {
        statusLines << "⚪ MONITORING";
    }

    if (!result.faces.isEmpty()) {
        statusLines << QString("👤 %1 Face(s)").arg(result.faces.size());
    }

    // 绘制状态背景
    QFont font = painter.font();
    font.setPixelSize(14);
    font.setBold(true);
    painter.setFont(font);

    QFontMetrics fm(font);
    int maxWidth = 0;
    int totalHeight = 0;

    for (const QString& line : statusLines) {
        QRect lineRect = fm.boundingRect(line);
        maxWidth = qMax(maxWidth, lineRect.width());
        totalHeight += lineRect.height() + 6;
    }

    QRect statusRect(imageSize.width() - maxWidth - 30, 15, maxWidth + 20, totalHeight + 15);
    painter.fillRect(statusRect, m_backgroundColor);

    // 绘制边框
    QPen borderPen(result.hasMotion ? m_motionBoxColor : QColor(128, 128, 128), 2);
    painter.setPen(borderPen);
    painter.drawRect(statusRect);

    // 绘制状态文字
    painter.setPen(QPen(m_textColor));
    int y = statusRect.top() + 25;
    for (const QString& line : statusLines) {
        painter.drawText(statusRect.left() + 10, y, line);
        y += fm.height() + 6;
    }
}

void DetectionVisualizer::drawTimestamp(QPainter& painter, const QDateTime& timestamp,
                                        const QSize& imageSize)
{
    QString timeStr = timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz");

    QFont font = painter.font();
    font.setPixelSize(12);
    painter.setFont(font);

    QFontMetrics fm(font);
    QRect textRect = fm.boundingRect(timeStr);
    textRect.moveBottomLeft(QPoint(15, imageSize.height() - 15));
    textRect.adjust(-8, -4, 8, 4);

    painter.fillRect(textRect, m_backgroundColor);
    painter.setPen(QPen(m_textColor));
    painter.drawText(textRect, Qt::AlignCenter, timeStr);
}
