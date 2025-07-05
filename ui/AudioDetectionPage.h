#ifndef AUDIODETECTIONPAGE_H
#define AUDIODETECTIONPAGE_H
#include "../widgets/RecordingDialog.h"
#include "../widgets/WaveFormManager.h"
#include "../widgets/AudioUploader.h"

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QMediaPlayer>
#include <QVector>
#include <atomic>
#include <QAudioDecoder>
#include <QListWidget>

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)
QT_FORWARD_DECLARE_CLASS(QHBoxLayout)

class AudioDetectionPage : public QWidget
{
    Q_OBJECT

public:
    explicit AudioDetectionPage(QWidget* parent = nullptr);

signals:
    void returnRequested();

private:
    std::atomic<bool> m_shouldStop{false};
    RecordingDialog* recordingDialog;
    // 控件声明
    QPushButton* backButton;
    QPushButton* detectButton;
    QPushButton* recordButton;
    QPushButton* uploadButton;
    QVector<qint16> audioData;

    QSharedPointer<QAudioDecoder> decoder;

    // 布局声明
    QVBoxLayout* mainLayout;
    QVBoxLayout* contentLayout;

    // 子部件
    QWidget* contentWidget;
    QLabel* formatLabel;

    QLabel* tipLabel;
    QPushButton* iconLabelBtn;
    QPushButton* playButton;
    QMediaPlayer* mediaPlayer;
    QString currentAudioPath;
    QString wavFilePath;
    AudioUploader uploader; 
    bool isPlaying = false;

    void initStyles();
    void initUpperSection();
    void initLowerSection();
    void populateFileList(QListWidget* listWidget, const QString& path);

private:
    WaveformManager *m_waveformManager;  // 波形管理对象
    QLabel *waveformDisplay;            


private slots:
    void handleRecord();
    void handleFileUpload();
    void handleAudioDetection();
    void togglePlayback();
    void onUploadFinished(bool success, const QString &message);
    bool convertPcmToWav();
    void resetWaveformDisplay();
    void onRecordingSaved(const QString& filePath);
};

#endif // AUDIODETECTIONPAGE_H
