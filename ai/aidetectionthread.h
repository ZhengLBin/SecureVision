#ifndef AIDETECTIONTHREAD_H
#define AIDETECTIONTHREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QImage>
#include "aitypes.h"
#include "motiondetector.h"
#include "facerecognitionmanager.h"

class AIDetectionThread : public QThread
{
    Q_OBJECT

public:
    explicit AIDetectionThread(QObject *parent = nullptr);
    ~AIDetectionThread() override;

    // 控制接口
    void startDetection();
    void stopDetection();
    AIConfig getConfig();
    void setConfig(const AIConfig& config);

    // 输入接口
    void addFrame(const QImage& frame);

    //  人脸识别相关方法
    void setFaceRecognitionEnabled(bool enabled);
    bool isFaceRecognitionEnabled() const { return m_faceRecognitionEnabled; }

    //  人脸管理方法
    bool registerFace(const QString& name, const QImage& faceImage);
    QStringList getRegisteredFaces();
    bool deleteFace(const QString& name);

signals:
    void detectionResult(const DetectionResult& result);
    void recordTrigger(RecordTrigger trigger, const QImage& frame);

protected:
    void run() override;

private:
    // 线程控制
    bool m_running = false;
    QMutex m_mutex;

    // 配置和状态
    AIConfig m_config;
    int m_frameCounter = 0;

    // 输入队列
    QQueue<QImage> m_frameQueue;
    static const int MAX_QUEUE_SIZE = 3;

    // 检测器
    MotionDetector* m_motionDetector;
    // RKNNFaceDetector* m_faceDetector;  // 下个阶段添加

    // 处理函数
    DetectionResult processFrame(const QImage& frame);
    bool shouldRecord(const DetectionResult& result);
    void clearQueue();

    //  人脸识别相关
    FaceRecognitionManager* m_faceManager;
    bool m_faceRecognitionEnabled;
    int m_faceDetectionFrameCounter;

    //  初始化方法
    bool initializeFaceRecognition();

    //  处理方法
    DetectionResult processFrameWithFaces(const QImage& frame);
    bool shouldProcessFaces();

private slots:
    //  人脸识别信号处理
    void onFaceDetected(const QVector<FaceInfo>& faces);
    void onFaceRecognized(const QString& name, float similarity);
    void onUnknownFaceDetected(const QRect& faceRect);
};

#endif // AIDETECTIONTHREAD_H
