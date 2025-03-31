#ifndef RECORDINGDIALOG_H
#define RECORDINGDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QElapsedTimer>
#include <rkmedia/rkmedia_api.h>
#include <rkmedia/rkmedia_buffer.h>
#include <fstream>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

class RecordingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RecordingDialog(QWidget* parent = nullptr);
    ~RecordingDialog();

signals:
    void recordingSaved(const QString& filePath);

private slots:
    void toggleRecording();
    void stopRecording();
    void updateTimer();

private:
    void startRecording();
    bool initRKMediaAudio();

    QTimer* timer;
    QElapsedTimer recordTimer;
    qint64 elapsedTime = 0;     // 已经过去的时间(毫秒)

    QLabel* timeLabel;
    QPushButton* toggleButton;
    QPushButton* stopButton;

    bool isRecording = false;
    bool isFirstStart = true;
    bool isRKMediaInitialized = false;
    AI_CHN micChn = 0;

    std::ofstream wavFile;
    std::string savedFilePath;
    uint32_t totalDataSize = 0;   


#pragma pack(push, 1)
    struct WAVHeader {
        char riff[4] = {'R','I','F','F'};
        uint32_t fileSize = 0;
        char wave[4] = {'W','A','V','E'};
        char fmt[4] = {'f','m','t',' '};
        uint32_t fmtSize = 16;
        uint16_t audioFormat = 1;     // PCM
        uint16_t numChannels = 2;     // 立体声
        uint32_t sampleRate = 16000;  // 16kHz
        uint32_t byteRate = 64000;    // 16000*2*2
        uint16_t blockAlign = 4;      // 2*2
        uint16_t bitsPerSample = 16;
        char data[4] = {'d','a','t','a'};
        uint32_t dataSize = 0;
    };
#pragma pack(pop)
};

#endif // RECORDINGDIALOG_H
