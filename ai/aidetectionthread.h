// ai/AIDetectionThread.h
#ifndef AIDETECTIONTHREAD_H
#define AIDETECTIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include <QTimer>
#include <QDateTime>

// 现有包含
#include "aitypes.h"
#include "motiondetector.h"
#include "facedatabase.h"

// 🆕 新增包含
#include "facerecognitionmanager.h"  // 人脸识别管理器

class AIDetectionThread : public QThread
{
    Q_OBJECT

public:
    explicit AIDetectionThread(QObject *parent = nullptr);
    ~AIDetectionThread();

    // 🔧 现有方法保持不变
    void startDetection();
    void stopDetection();
    void addFrame(const QImage& frame);
    void setConfig(const AIConfig& config);
    AIConfig getConfig();
    void clearQueue();

    // 🆕 人脸识别相关方法
    void setFaceRecognitionEnabled(bool enabled);
    bool isFaceRecognitionEnabled() const { return m_faceRecognitionEnabled; }

    void runSimpleFaceTest();

    // 🆕 人脸管理方法
    bool registerFace(const QString& name, const QImage& faceImage);
    QStringList getRegisteredFaces();
    bool deleteFace(const QString& name);
    int getTotalRegisteredFaces();

    // 🆕 获取人脸识别管理器（用于外部访问）
    FaceRecognitionManager* getFaceRecognitionManager() const { return m_faceManager; }

    // 🆕 配置方法
    void setFaceDetectionThreshold(float threshold);
    void setFaceRecognitionThreshold(float threshold);

signals:
    // 现有信号保持不变
    void detectionResult(const DetectionResult& result);
    void recordTrigger(RecordTrigger trigger, const QImage& frame);

    // 🆕 人脸识别相关信号
    void faceDetectionStatusChanged(bool enabled);
    void faceRegistered(const QString& name);
    void faceRecognitionError(const QString& error);
    void performanceUpdate(const QString& info);

protected:
    void run() override;

private slots:
    // 🆕 人脸识别信号处理槽
    void onFaceDetected(const QVector<FaceInfo>& faces);
    void onFaceRecognized(const QString& name, float similarity);
    void onUnknownFaceDetected(const QRect& faceRect);
    void onFaceManagerInitializationFailed(const QString& reason);

private:
    // 🔧 现有成员变量保持不变
    MotionDetector* m_motionDetector;
    FaceDatabase* m_faceDatabase;

    // 线程控制
    QMutex m_mutex;
    volatile bool m_running = false;
    QQueue<QImage> m_frameQueue;
    int m_frameCounter = 0;

    // 配置和状态
    AIConfig m_config;
    DetectionResult m_lastResult;

    // 常量
    static const int MAX_QUEUE_SIZE = 10;

    // 🆕 人脸识别相关成员变量
    FaceRecognitionManager* m_faceManager;
    bool m_faceRecognitionEnabled;
    int m_faceDetectionFrameCounter;


    void testFeatureConsistency(const QImage& testImage, const QString& userName);

    // 🆕 性能统计
    QDateTime m_lastFaceDetectionTime;
    int m_totalFaceDetections;
    int m_totalFaceRecognitions;

    // 🔧 现有私有方法保持不变
    DetectionResult processFrame(const QImage& frame);
    bool shouldRecord(const DetectionResult& result);
    void testFaceDatabase();

    // 🆕 人脸识别相关私有方法
    bool initializeFaceRecognition();
    DetectionResult processFrameWithFaces(const QImage& frame);
    bool shouldProcessFaces();
    void updateFaceDetectionStatistics(const DetectionResult& result);
    void logFaceDetectionPerformance();
};

#endif // AIDETECTIONTHREAD_H
