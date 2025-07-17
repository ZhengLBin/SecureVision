#include "recordmanager.h"

RecordManager::RecordManager(QObject *parent)
    : QObject(parent)
{
    m_recordTimer = new QTimer(this);
    m_recordTimer->setSingleShot(true);
    connect(m_recordTimer, &QTimer::timeout, this, &RecordManager::onRecordTimer);

    m_cooldownTimer = new QTimer(this);
    m_cooldownTimer->setSingleShot(true);
    connect(m_cooldownTimer, &QTimer::timeout, this, &RecordManager::onCooldownTimer);
}

RecordManager::~RecordManager()
{
    if (m_state == Recording) {
        stopActualRecording();
    }
}

void RecordManager::onMotionDetected()
{
    if (!m_enabled) return;

    m_lastMotionTime = QDateTime::currentDateTime();

    switch (m_state) {
    case Idle:
        changeState(PreRecord);
        m_recordTimer->start(m_preRecordDelay);
        qDebug() << "RecordManager: Motion detected - Starting pre-record timer";
        break;

    case PreRecord:
        // 继续等待，重置计时器
        m_recordTimer->start(m_preRecordDelay);
        break;

    case Recording:
        // 正在录制，重置后录制计时器
        m_recordTimer->start(m_postRecordDelay);
        qDebug() << "RecordManager: Motion continues - Extending recording";
        break;

    case PostRecord:
        // 重新进入录制状态
        changeState(Recording);
        m_recordTimer->start(m_postRecordDelay);
        qDebug() << "RecordManager: Motion resumed - Back to recording";
        break;

    case Cooldown:
        qDebug() << "RecordManager: Motion detected during cooldown - Ignored";
        break;
    }
}

void RecordManager::onRecordTimer()
{
    switch (m_state) {
    case PreRecord:
        // 开始正式录制
        changeState(Recording);
        startActualRecording();
        m_recordTimer->start(m_postRecordDelay);
        qDebug() << "RecordManager: Starting actual recording";
        break;

    case Recording:
    case PostRecord:
        // 检查是否达到最小录制时长
        int recordedDuration = m_recordStartTime.msecsTo(QDateTime::currentDateTime());
        if (recordedDuration >= m_minRecordDuration) {
            stopActualRecording();
            changeState(Cooldown);
            m_cooldownTimer->start(m_cooldownPeriod);
            qDebug() << "RecordManager: Recording stopped - Entering cooldown";
        } else {
            // 未达到最小时长，继续录制
            m_recordTimer->start(1000); // 1秒后再检查
            qDebug() << "RecordManager: Minimum duration not reached, continuing...";
        }
        break;
    }
}

void RecordManager::onCooldownTimer()
{
    changeState(Idle);
    qDebug() << "RecordManager: Cooldown finished - Ready for next recording";
}

void RecordManager::startActualRecording()
{
    m_recordStartTime = QDateTime::currentDateTime();
    m_currentRecordFile = generateRecordFilename();

    // TODO: 实际启动视频录制
    // 这里需要集成实际的视频编码器

    emit recordingStarted(m_currentRecordFile);
}

void RecordManager::stopActualRecording()
{
    if (m_state == Recording) {
        int duration = m_recordStartTime.msecsTo(QDateTime::currentDateTime());

        // TODO: 停止实际的视频录制

        emit recordingStopped(m_currentRecordFile, duration);
        m_currentRecordFile.clear();
    }
}

QString RecordManager::generateRecordFilename()
{
    return QString("motion_%1.mp4").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
}

void RecordManager::changeState(RecordState newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit recordingStateChanged(newState);
    }
}

void RecordManager::onFaceDetected()
{
    if (!m_enabled) return;

    // 人脸检测触发录制的逻辑与运动检测相同
    // 因为人脸检测通常比运动检测更精确
    onMotionDetected();

    qDebug() << "RecordManager: Face detected - triggering recording";
}

void RecordManager::onManualTrigger()
{
    if (!m_enabled) return;

    // 手动触发立即开始录制，跳过预录制阶段
    if (m_state == Idle || m_state == Cooldown) {
        changeState(Recording);
        startActualRecording();
        m_recordTimer->start(m_minRecordDuration); // 手动录制使用最小时长
        qDebug() << "RecordManager: Manual trigger - Starting immediate recording";
    } else {
        qDebug() << "RecordManager: Manual trigger ignored - Already recording";
    }
}

void RecordManager::stopRecording()
{
    // 强制停止录制
    if (m_state == Recording || m_state == PostRecord) {
        m_recordTimer->stop();
        stopActualRecording();
        changeState(Idle);
        qDebug() << "RecordManager: Recording manually stopped";
    } else {
        qDebug() << "RecordManager: Stop recording called but not recording";
    }
}
