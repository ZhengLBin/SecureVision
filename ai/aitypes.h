#ifndef AI_TYPES_H
#define AI_TYPES_H

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
    // 现有字段保持不变
    QVector<QRect> faces;           // 人脸检测框（保持向后兼容）
    QVector<float> confidences;     // 置信度（保持向后兼容）
    QDateTime timestamp;            // 检测时间
    bool hasMotion;                 // 是否有移动
    QRect motionArea;              // 移动区域

    // 🆕 新增人脸检测相关字段
    QVector<FaceInfo> faceInfos;        // 详细的人脸信息
    bool hasFaceDetection = false;      // 是否检测到人脸
    int totalFaceCount = 0;             // 总人脸数量
    int recognizedFaceCount = 0;        // 已识别人脸数量
    int unknownFaceCount = 0;           // 未知人脸数量

    // 🆕 性能统计
    float motionProcessTime = 0.0f;     // 运动检测耗时(ms)
    float faceDetectionTime = 0.0f;     // 人脸检测耗时(ms)
    float faceRecognitionTime = 0.0f;   // 人脸识别耗时(ms)
};

// 🔧 扩展现有的AIConfig结构体
struct AIConfig {
    // 现有字段保持不变
    bool enableAI = false;              // AI开关
    bool enableMotionDetect = true;     // 移动检测开关
    bool enableFaceDetect = true;       // 人脸检测开关
    float motionThreshold = 0.3f;       // 移动检测阈值
    float faceThreshold = 0.5f;         // 人脸检测阈值
    int skipFrames = 3;                 // 跳帧数量
    QRect roiArea;                      // 感兴趣区域

    // 🆕 新增人脸识别相关配置
    bool enableFaceRecognition = true;      // 启用人脸识别
    float faceRecognitionThreshold = 0.7f;  // 人脸识别相似度阈值
    int faceDetectionInterval = 2;          // 人脸检测间隔（帧数）
    bool recordUnknownFaces = true;         // 记录未知人脸
    bool recordKnownFaces = false;          // 记录已知人脸
    QString faceModelPath = "/demo/src/rockx_data";  // RockX模型路径

    // 🆕 性能优化配置
    int maxImageWidth = 640;                // 最大处理图像宽度
    int maxImageHeight = 480;               // 最大处理图像高度
    bool enablePerformanceLogging = false;  // 启用性能日志
};

// 🔧 扩展现有的RecordTrigger枚举
enum class RecordTrigger {
    None,
    MotionDetected,        // 保持现有
    FaceDetected,          // 保持现有
    ManualTrigger,         // 保持现有
    UnknownFaceDetected,   // 🆕 新增：未知人脸触发
    KnownFaceDetected,     // 🆕 新增：已知人脸触发
    MultipleFacesDetected  // 🆕 新增：多人脸触发
};

// 🆕 人脸数据库记录结构体
struct FaceRecord {
    int id;                    // 数据库ID
    QString name;              // 人员姓名
    QString imagePath;         // 图像文件路径
    QDateTime createTime;      // 创建时间
    QDateTime lastSeen;        // 最后识别时间
    int recognitionCount;      // 识别次数
    QString description;       // 描述信息
    bool isActive;            // 是否激活

    FaceRecord() : id(-1), recognitionCount(0), isActive(true) {}
};

// 🆕 人脸识别配置状态
enum class FaceRecognitionStatus {
    NotInitialized,    // 未初始化
    Initializing,      // 初始化中
    Ready,            // 就绪
    Error,            // 出错
    Disabled          // 已禁用
};

Q_DECLARE_METATYPE(DetectionResult)
Q_DECLARE_METATYPE(RecordTrigger)
Q_DECLARE_METATYPE(FaceInfo)
Q_DECLARE_METATYPE(FaceRecord)
Q_DECLARE_METATYPE(FaceRecognitionStatus)

#endif // AI_TYPES_H
