#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QImage>
#include <QDebug>
#include "../ai/aitypes.h"

class RecordManager : public QObject
{
    Q_OBJECT

public:
    enum RecordState {
        Idle,           // 空闲状态
        PreRecord,      // 预录制（检测到运动，准备开始）
        Recording,      // 正在录制
        PostRecord,     // 后录制（运动停止，延迟录制）
        Cooldown        // 冷却期
    };

    explicit RecordManager(QObject *parent = nullptr);
    ~RecordManager();

    // 控制接口
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    bool isRecording() const { return m_state == Recording; }
    RecordState currentState() const { return m_state; }

    // 配置接口
    void setPreRecordDelay(int ms) { m_preRecordDelay = ms; }
    void setPostRecordDelay(int ms) { m_postRecordDelay = ms; }
    void setMinRecordDuration(int ms) { m_minRecordDuration = ms; }
    void setCooldownPeriod(int ms) { m_cooldownPeriod = ms; }
    void setMaxRecordDuration(int ms) { m_maxRecordDuration = ms; }

public slots:
    void onMotionDetected();
    void onFaceDetected();
    void onManualTrigger();
    void stopRecording();

signals:
    void recordingStarted(const QString& filename);
    void recordingStopped(const QString& filename, int duration);
    void recordingStateChanged(RecordState state);

private slots:
    void onRecordTimer();
    void onCooldownTimer();

private:
    bool m_enabled = true;
    RecordState m_state = Idle;

    QTimer* m_recordTimer;
    QTimer* m_cooldownTimer;
    QDateTime m_lastMotionTime;
    QDateTime m_recordStartTime;
    QString m_currentRecordFile;

    // 配置参数
    int m_preRecordDelay = 1000;      // 预录制延迟1秒
    int m_postRecordDelay = 5000;     // 运动停止后继续录制5秒
    int m_minRecordDuration = 10000;  // 最小录制时长10秒
    int m_cooldownPeriod = 3000;      // 冷却期3秒
    int m_maxRecordDuration = 300000; // 最大录制时长5分钟

    void startActualRecording();
    void stopActualRecording();
    QString generateRecordFilename();
    void changeState(RecordState newState);
};

#endif // RECORDMANAGER_H
