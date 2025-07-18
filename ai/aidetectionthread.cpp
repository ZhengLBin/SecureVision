#include "aidetectionthread.h"
#include <QDebug>
#include <QMutexLocker>

AIDetectionThread::AIDetectionThread(QObject *parent)
    : QThread(parent)
    , m_motionDetector(new MotionDetector(this))
{
}

AIDetectionThread::~AIDetectionThread()
{
    stopDetection();
    if (isRunning()) {
        quit();
        wait(3000);
    }
}

void AIDetectionThread::startDetection()
{
    QMutexLocker locker(&m_mutex);
    m_running = true;
    if (!isRunning()) {
        start();
    }
    qDebug() << "AI Detection started";
}

void AIDetectionThread::stopDetection()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    qDebug() << "AI Detection stopped";
}

AIConfig AIDetectionThread::getConfig() {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void AIDetectionThread::setConfig(const AIConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;

    // 更新检测器配置
    if (m_motionDetector) {
        m_motionDetector->setThreshold(config.motionThreshold);
        m_motionDetector->setROI(config.roiArea);
    }
}

void AIDetectionThread::addFrame(const QImage& frame)
{
    QMutexLocker locker(&m_mutex);

    if (!m_config.enableAI || !m_running) return;

    // 跳帧处理
    if (++m_frameCounter % (m_config.skipFrames + 1) != 0) return;

    // 队列管理
    if (m_frameQueue.size() >= MAX_QUEUE_SIZE) {
        m_frameQueue.dequeue(); // 丢弃旧帧
    }

    m_frameQueue.enqueue(frame);
}

void AIDetectionThread::run()
{
    qDebug() << "AIDetectionThread started";

    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running) break;
        }

        QImage frame;
        {
            QMutexLocker locker(&m_mutex);
            if (m_frameQueue.isEmpty()) {
                locker.unlock();
                msleep(33); // ~30fps
                continue;
            }
            frame = m_frameQueue.dequeue();
        }

        // 处理帧
        DetectionResult result = processFrame(frame);
        emit detectionResult(result);

        // 检查是否需要录制
        if (shouldRecord(result)) {
            RecordTrigger trigger = RecordTrigger::None;
            if (result.hasMotion) trigger = RecordTrigger::Motion;
            if (!result.faces.isEmpty()) trigger = RecordTrigger::Face;

            emit recordTrigger(trigger, frame);
        }
    }

    qDebug() << "AIDetectionThread finished";
}

bool AIDetectionThread::initializeFaceRecognition()
{
    m_faceManager = new FaceRecognitionManager(this);

    // 连接信号
    connect(m_faceManager, &FaceRecognitionManager::faceDetected,
            this, &AIDetectionThread::onFaceDetected);
    connect(m_faceManager, &FaceRecognitionManager::faceRecognized,
            this, &AIDetectionThread::onFaceRecognized);
    connect(m_faceManager, &FaceRecognitionManager::unknownFaceDetected,
            this, &AIDetectionThread::onUnknownFaceDetected);

    // 初始化人脸识别管理器
    if (!m_faceManager->initialize()) {
        qDebug() << "Failed to initialize face recognition manager";
        delete m_faceManager;
        m_faceManager = nullptr;
        return false;
    }

    qDebug() << "Face recognition initialized successfully";
    return true;
}

DetectionResult AIDetectionThread::processFrame(const QImage& frame)
{
    DetectionResult result;
    result.timestamp = QDateTime::currentDateTime();

    QTime totalTimer;
    totalTimer.start();

    // 1. 运动检测 (现有逻辑)
    if (m_config.enableMotionDetect && m_motionDetector) {
        QTime motionTimer;
        motionTimer.start();

        QRect motionArea;
        result.hasMotion = m_motionDetector->detectMotion(frame, motionArea);
        result.motionArea = motionArea;

        result.motionProcessTime = motionTimer.elapsed();
    }

    // 2. 🆕 人脸检测和识别
    if (m_config.enableFaceDetect && m_faceManager && shouldProcessFaces()) {
        QTime faceTimer;
        faceTimer.start();

        if (m_config.enableFaceRecognition) {
            // 执行检测+识别
            result.faces = m_faceManager->detectAndRecognizeFaces(frame);
        } else {
            // 仅执行检测
            result.faces = m_faceManager->detectFaces(frame);
        }

        // 更新统计信息
        result.hasFaceDetection = !result.faces.isEmpty();
        result.totalFaceCount = result.faces.size();
        result.recognizedFaceCount = 0;

        for (const auto& face : result.faces) {
            if (face.isRecognized) {
                result.recognizedFaceCount++;
            }
        }

        result.faceDetectionTime = faceTimer.elapsed();

        // 触发录制逻辑
        if (result.hasFaceDetection) {
            if (result.recognizedFaceCount > 0 && m_config.recordKnownFaces) {
                emit recordTrigger(RecordTrigger::Face, frame);
            } else if (result.recognizedFaceCount < result.totalFaceCount && m_config.recordUnknownFaces) {
                emit recordTrigger(RecordTrigger::UnknownFace, frame);
            }
        }
    }

    return result;
}

bool AIDetectionThread::shouldProcessFaces()
{
    // 智能调度：根据运动检测结果调整人脸检测频率
    m_faceDetectionFrameCounter++;

    if (m_lastResult.hasMotion) {
        // 有运动时增加人脸检测频率
        return m_faceDetectionFrameCounter % 2 == 0;
    } else {
        // 无运动时降低人脸检测频率
        return m_faceDetectionFrameCounter % 5 == 0;
    }
}

// 🆕 人脸识别相关槽函数
void AIDetectionThread::onFaceDetected(const QVector<FaceInfo>& faces)
{
    qDebug() << QString("Detected %1 faces").arg(faces.size());
}

void AIDetectionThread::onFaceRecognized(const QString& name, float similarity)
{
    qDebug() << QString("Recognized face: %1 (similarity: %.2f)").arg(name).arg(similarity);
}

void AIDetectionThread::onUnknownFaceDetected(const QRect& faceRect)
{
    qDebug() << "Unknown face detected at:" << faceRect;
}

// 🆕 人脸管理API
bool AIDetectionThread::registerFace(const QString& name, const QImage& faceImage)
{
    if (!m_faceManager) {
        return false;
    }

    return m_faceManager->registerFace(name, faceImage);
}

QStringList AIDetectionThread::getRegisteredFaces()
{
    if (!m_faceManager) {
        return QStringList();
    }

    return m_faceManager->getAllRegisteredNames();
}

bool AIDetectionThread::shouldRecord(const DetectionResult& result)
{
    // 当前阶段：有移动就录制
    return result.hasMotion;

    // 下个阶段：有人脸就录制
    // return !result.faces.isEmpty();
}

void AIDetectionThread::clearQueue()
{
    QMutexLocker locker(&m_mutex);
    m_frameQueue.clear();
}
