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
            if (result.hasMotion) trigger = RecordTrigger::MotionDetected;
            if (!result.faces.isEmpty()) trigger = RecordTrigger::FaceDetected;

            emit recordTrigger(trigger, frame);
        }
    }

    qDebug() << "AIDetectionThread finished";
}

DetectionResult AIDetectionThread::processFrame(const QImage& frame)
{
    DetectionResult result;
    result.timestamp = QDateTime::currentDateTime();

    // 移动检测
    if (m_config.enableMotionDetect) {
        QRect motionArea;
        result.hasMotion = m_motionDetector->detectMotion(frame, motionArea);
        result.motionArea = motionArea;

        // 添加调试输出
        if (result.hasMotion) {
            qDebug() << "AIDetectionThread: Motion detected at"
                     << result.timestamp.toString("hh:mm:ss.zzz")
                     << "Area:" << result.motionArea;
        }
    }

    // 人脸检测 (暂时模拟，下个阶段实现)
    if (m_config.enableFaceDetect && result.hasMotion) {
        // TODO: 添加RKNN人脸检测
        // result.faces = m_faceDetector->detect(frame);
    }

    return result;
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
