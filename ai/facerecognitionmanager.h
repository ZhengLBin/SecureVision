// ai/FaceRecognitionManager.h
#ifndef FACERECOGNITIONMANAGER_H
#define FACERECOGNITIONMANAGER_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include <QDebug>
#include <rockx.h>
#include "aitypes.h"
#include "facedatabase.h"

class FaceRecognitionManager : public QObject
{
    Q_OBJECT

public:
    explicit FaceRecognitionManager(QObject *parent = nullptr);
    ~FaceRecognitionManager();

    // 🔧 初始化和配置
    bool initialize(const QString& modelPath = "");
    bool isInitialized() const { return m_initialized; }
    void setDetectionThreshold(float threshold) { m_detectionThreshold = threshold; }
    void setRecognitionThreshold(float threshold) { m_recognitionThreshold = threshold; }

    // 🎯 核心功能接口
    QVector<FaceInfo> detectFaces(const QImage& image);
    QVector<FaceInfo> detectAndRecognizeFaces(const QImage& image);

    // 👤 人脸管理接口
    bool registerFace(const QString& name, const QImage& faceImage);
    QStringList getAllRegisteredNames();
    int getTotalRegisteredFaces();

    // 📊 性能统计
    float getLastDetectionTime() const { return m_lastDetectionTime; }
    float getLastRecognitionTime() const { return m_lastRecognitionTime; }

    void testBasicFunctionality();

signals:
    void faceDetected(const QVector<FaceInfo>& faces);
    void faceRecognized(const QString& name, float similarity);
    void unknownFaceDetected(const QRect& faceRect);
    void initializationFailed(const QString& reason);

private:
    // 🔧 RockX句柄（参考官方示例）
    rockx_handle_t m_faceDetHandle;
    rockx_handle_t m_faceLandmarkHandle;  // 5点关键点
    rockx_handle_t m_faceRecognizeHandle;

    // 📊 状态管理
    bool m_initialized;
    mutable QMutex m_mutex;
    QString m_modelPath;

    // ⚙️ 配置参数
    float m_detectionThreshold;
    float m_recognitionThreshold;

    // 🗄️ 数据库管理
    FaceDatabase* m_database;

    // 📈 性能统计
    float m_lastDetectionTime;
    float m_lastRecognitionTime;
    int m_detectionCount;
    int m_recognitionCount;

    // 🔨 私有方法
    bool initializeRockX();
    void cleanup();
    bool validateModelFiles(const QString& modelPath);

    // 🖼️ 图像处理方法
    rockx_image_t qImageToRockxImage(const QImage& image);
    QImage preprocessImage(const QImage& image);

    // 🎯 核心算法方法
    QVector<FaceInfo> processDetection(const QImage& image);
    FaceInfo processRecognition(const FaceInfo& detectedFace, const QImage& originalImage);
    QByteArray extractFaceFeature(const QImage& faceImage);

    // 🛠️ 辅助方法
    rockx_object_t* getMaxFace(rockx_object_array_t* faceArray);
    void logPerformance(const QString& operation, float timeMs);
};

#endif // FACERECOGNITIONMANAGER_H
