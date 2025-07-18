// ai/AITypes.h (扩展现有文件)
#ifndef AITYPES_H
#define AITYPES_H

#include <QRect>
#include <QImage>
#include <QDateTime>
#include <QVector>
#include <QString>

// 🆕 人脸信息结构体
struct FaceInfo {
    QRect bbox;              // 人脸边界框
    float confidence;        // 检测置信度
    QString personName;      // 识别到的人名（空表示未识别）
    int faceId;             // 人脸ID（数据库中的ID）
    bool isRecognized;      // 是否成功识别
    float similarity;       // 相似度分数

    FaceInfo() : confidence(0.0f), faceId(-1), isRecognized(false), similarity(0.0f) {}
};

// 🔧 扩展现有的DetectionResult结构体
struct DetectionResult {
    // 现有字段
    QDateTime timestamp;
    bool hasMotion = false;
    QRect motionArea;

    // 🆕 人脸检测相关字段
    QVector<FaceInfo> faces;        // 检测到的人脸信息
    bool hasFaceDetection = false;  // 是否检测到人脸
    int totalFaceCount = 0;         // 总人脸数量
    int recognizedFaceCount = 0;    // 已识别人脸数量

    // 🆕 性能统计
    float motionProcessTime = 0.0f;    // 运动检测耗时(ms)
    float faceDetectionTime = 0.0f;    // 人脸检测耗时(ms)
    float faceRecognitionTime = 0.0f;  // 人脸识别耗时(ms)
};

// 录制触发类型 (现有)
enum class RecordTrigger {
    None,
    Motion,
    Face,           // 🆕 人脸检测触发
    UnknownFace,    // 🆕 未知人脸触发
    Manual
};

// 🆕 AI配置结构体扩展
struct AIConfig {
    // 基础配置
    bool enableAI = false;
    bool enableMotionDetect = true;
    bool enableFaceDetect = true;        // 🆕 启用人脸检测
    bool enableFaceRecognition = true;   // 🆕 启用人脸识别

    // 阈值配置
    float motionThreshold = 0.3f;
    float faceDetectionThreshold = 0.5f;     // 🆕 人脸检测置信度阈值
    float faceRecognitionThreshold = 0.7f;   // 🆕 人脸识别相似度阈值

    // 性能配置
    int skipFrames = 3;                      // 跳帧数量
    int faceDetectionInterval = 2;           // 🆕 人脸检测间隔

    // 区域配置
    QRect roiArea;                           // 感兴趣区域

    // 🆕 人脸识别配置
    bool recordUnknownFaces = true;          // 记录未知人脸
    bool recordKnownFaces = false;           // 记录已知人脸
    QString faceModelPath = "/demo/src/rockx_data";  // RockX模型路径
};

// 🆕 人脸数据库记录
struct FaceRecord {
    int id;
    QString name;
    QString imagePath;
    QDateTime createTime;
    QDateTime lastSeen;
    int recognitionCount;

    FaceRecord() : id(-1), recognitionCount(0) {}
};

#endif // AITYPES_H
