#ifndef AICONTROLWIDGET_H
#define AICONTROLWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFrame>
#include "../ai/aitypes.h"
#include "../capture/recordmanager.h"

class AIControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AIControlWidget(QWidget *parent = nullptr);

    // 状态更新接口
    void updateDetectionStatus(const DetectionResult& result);
    void updateRecordingStatus(RecordManager::RecordState state);

signals:
    void aiToggled(bool enabled);
    void visualizationToggled(bool enabled);
    void recordingToggled(bool enabled);
    void configChanged(const AIConfig& config);

private slots:
    void onAIToggled(bool enabled);
    void onVisualizationToggled(bool enabled);
    void onRecordingToggled(bool enabled);
    void onThresholdChanged(int value);

private:
    // UI组件
    QPushButton* m_aiButton;
    QCheckBox* m_visualizationCheckBox;
    QCheckBox* m_recordingCheckBox;
    QSlider* m_thresholdSlider;
    QLabel* m_thresholdLabel;
    QLabel* m_statusLabel;
    QLabel* m_recordStatusLabel;

    // 状态
    bool m_aiEnabled = false;
    bool m_visualizationEnabled = true;
    bool m_recordingEnabled = true;
    AIConfig m_currentConfig;

    // UI创建函数
    void setupUI();
    void updateButtonStyles();
    void updateStatusDisplay();
    void createAIToggleCard(QVBoxLayout* layout);
    void createFunctionSwitchCard(QVBoxLayout* layout);
    void createParameterCard(QVBoxLayout* layout);
    void createStatusCard(QVBoxLayout* layout);
};

#endif // AICONTROLWIDGET_H
