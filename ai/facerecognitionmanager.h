// ai/FaceRecognitionManager.h
#ifndef FACERECOGNITIONMANAGER_H
#define FACERECOGNITIONMANAGER_H

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include <rockx.h>
#include "aitypes.h"
#include "facedatabase.h"

class FaceRecognitionManager : public QObject
{
    Q_OBJECT

public:
    explicit FaceRecognitionManager(QObject *parent = nullptr);
    ~FaceRecognitionManager();

    // 初始化和配置
    bool initialize(const QString& modelPath = "/demo/src/rockx_data");
    bool isInitialized() const { return m_initialized; }

    // 配置参数
    void setDetectionThreshold(float threshold) { m_detectionThreshold = threshold; }
    void setRecognitionThreshold(float threshold) { m_recognitionThreshold = threshold; }

    // 核心功能
    QVector<FaceInfo> detectFaces(const QImage& image);
    QVector<FaceInfo> detectAndRecognizeFaces(const QImage& image);

    // 人脸管理
    bool registerFace(const QString& name, const QImage& faceImage);
    bool deleteFace(const QString& name);
    QStringList getAllRegisteredNames();
    int getTotalRegisteredFaces();

    // 性能统计
    float getLastDetectionTime() const { return m_lastDetectionTime; }
    float getLastRecognitionTime() const { return m_lastRecognitionTime; }
    int getDetectionCount() const { return m_detectionCount; }
    int getRecognitionCount() const { return m_recognitionCount; }

signals:
    void faceDetected(const QVector<FaceInfo>& faces);
    void faceRecognized(const QString& name, float similarity);
    void unknownFaceDetected(const QRect& faceRect);
    void initializationFailed(const QString& reason);
    void databaseError(const QString& error);

private slots:
    void onDatabaseError(const QString& error);

private:
    // RockX句柄
    rockx_handle_t m_faceDetHandle;
    rockx_handle_t m_faceRecognHandle;
    rockx_handle_t m_faceLandmarkHandle;

    // 状态管理
    bool m_initialized;
    QMutex m_mutex;

    // 配置参数
    float m_detectionThreshold;
    float m_recognitionThreshold;
    QString m_modelPath;

    // 数据库管理
    FaceDatabase* m_database;

    // 性能统计
    float m_lastDetectionTime;
    float m_lastRecognitionTime;
    int m_detectionCount;
    int m_recognitionCount;

    // 私有方法
    bool initializeRockX();
    void cleanup();
    QVector<FaceInfo> processDetection(const QImage& image);
    QVector<FaceInfo> processRecognition(const QVector<FaceInfo>& detectedFaces, const QImage& image);
    QImage preprocessImage(const QImage& image);
    rockx_image_t qImageToRockxImage(const QImage& image);
    QByteArray extractFaceFeature(const QImage& faceImage);
    FaceInfo recognizeSingleFace(const FaceInfo& detectedFace, const QImage& image);

    // 辅助函数
    bool setDataPath();
    void logPerformance(const QString& operation, float timeMs);
};

#endif // FACERECOGNITIONMANAGER_H
