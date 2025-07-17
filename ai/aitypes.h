#ifndef AI_TYPES_H
#define AI_TYPES_H

#include <QRect>
#include <QImage>
#include <QDateTime>
#include <QVector>

// AI检测结果
struct DetectionResult {
    QVector<QRect> faces;           // 人脸检测框
    QVector<float> confidences;     // 置信度
    QDateTime timestamp;            // 检测时间
    bool hasMotion;                 // 是否有移动
    QRect motionArea;              // 移动区域
};

// AI配置参数
struct AIConfig {
    bool enableAI = false;          // AI开关
    bool enableMotionDetect = true; // 移动检测开关
    bool enableFaceDetect = true;   // 人脸检测开关
    float motionThreshold = 0.3f;   // 移动检测阈值
    float faceThreshold = 0.5f;     // 人脸检测阈值
    int skipFrames = 3;             // 跳帧数量
    QRect roiArea;                  // 感兴趣区域
};

// 录制触发原因
enum class RecordTrigger {
    None,
    MotionDetected,
    FaceDetected,
    ManualTrigger
};

Q_DECLARE_METATYPE(DetectionResult)
Q_DECLARE_METATYPE(RecordTrigger)

#endif // AI_TYPES_H
