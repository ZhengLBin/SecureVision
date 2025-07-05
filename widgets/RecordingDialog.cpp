#include "RecordingDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QIcon>
#include <QStandardPaths>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QDateTime>
#include <QThread>

#define AI_CHN 0
#define AUDIO_PATH "default"

RecordingDialog::RecordingDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(u8"录音中");
    setFixedSize(500, 500);
    setWindowFlags(Qt::Popup);

    setAttribute(Qt::WA_DeleteOnClose);

    if (!initRKMediaAudio()) {
        QMessageBox::critical(this, "错误", "无法初始化音频设备！");
        return;
    }

    timer = new QTimer(this);
    timer->setInterval(10);
    connect(timer, &QTimer::timeout, this, &RecordingDialog::updateTimer);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    timeLabel = new QLabel("00:00.00");
    timeLabel->setStyleSheet("font-size: 32px; color: #409EFF;");
    timeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(timeLabel);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    toggleButton = new QPushButton();
    toggleButton->setIcon(QIcon(":/AVDeepfake/startRecord"));
    toggleButton->setCursor(Qt::PointingHandCursor);
    toggleButton->setIconSize(QSize(64, 64));
    toggleButton->setStyleSheet("QPushButton { border: none; }");
    connect(toggleButton, &QPushButton::clicked, this, &RecordingDialog::toggleRecording);
    buttonLayout->addWidget(toggleButton);

    stopButton = new QPushButton();
    stopButton->setIcon(QIcon(":/AVDeepfake/endRecord"));
    stopButton->setCursor(Qt::PointingHandCursor);
    stopButton->setIconSize(QSize(64, 64));
    stopButton->setStyleSheet("QPushButton { border: none; }");
    connect(stopButton, &QPushButton::clicked, this, &RecordingDialog::stopRecording);
    stopButton->hide(); // 初始隐藏停止按钮
    buttonLayout->addWidget(stopButton);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);
}

RecordingDialog::~RecordingDialog()
{
    if (isRKMediaInitialized) {
        qWarning() << "~isRKMediaInitialized---" << isRKMediaInitialized;
        if (isRecording) {
            // 停止接收缓冲
            RK_MPI_SYS_StopGetMediaBuffer(RK_ID_AI, AI_CHN);
            qWarning() << "RK_MPI_SYS_StopGetMediaBuffer Already" << AI_CHN;
            isRecording = false;
        }
        // 禁用音频通道
        RK_MPI_AI_DisableChn(AI_CHN);
        qWarning() << "RK_MPI_AI_DisableChn Already" << AI_CHN;
    }

    // 确保文件关闭
    if (wavFile.is_open()) {
        wavFile.close();
    }

        // 确保所有线程退出

}

bool RecordingDialog::initRKMediaAudio() {
    // 初始化RKMedia系统
    if (isRKMediaInitialized) {
        qWarning() << "RKMedia Init Already";
        return true;
    }
    qDebug() << "isRKMediaInitialized---------" << isRKMediaInitialized;
    // 初始化 RKMedia 系统  

    // 设置AI通道属性
    AI_CHN_ATTR_S ai_chn_s = {};  // 按照示例代码风格
    ai_chn_s.pcAudioNode = const_cast<RK_CHAR*>(AUDIO_PATH);
    ai_chn_s.u32SampleRate = 16000;
    ai_chn_s.u32NbSamples = 1024;
    ai_chn_s.u32Channels = 2;
    ai_chn_s.enSampleFormat = RK_SAMPLE_FMT_S16;
    ai_chn_s.enAiLayout = AI_LAYOUT_NORMAL;

    int ret = RK_MPI_AI_SetChnAttr(AI_CHN, &ai_chn_s);
    if (ret != 0) {
        qWarning() << "RK_MPI_AI_SetChnAttr fail";
        return false;
    }

    ret = RK_MPI_AI_EnableChn(AI_CHN);
    if (ret != 0) {
        qWarning() << "RK_MPI_AI_EnableChn fail";
        return false;
    }

    // 记录通道号确保一致性
    micChn = AI_CHN;
    
    // 设置音量 (添加这部分)
    int s32Volume = 100; // 设置一个较高的音量值(0-100)
    if (RK_MPI_AI_SetVolume(micChn, s32Volume) != 0) {
        qWarning() << "RK_MPI_AI_SetVolume fail";
    }
    isRKMediaInitialized = true;
    return true;
}

void RecordingDialog::toggleRecording() {
    if (!isRecording) {
        startRecording();
    }
}

void RecordingDialog::startRecording() {
    if (!isRKMediaInitialized) return;

    if (isFirstStart) {
        QString fileName = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".pcm";
        QString savePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                         + "/recordings/" + fileName;
        QDir().mkpath(QFileInfo(savePath).absolutePath());

        wavFile.open(savePath.toStdString(), std::ios::binary);
        if (!wavFile.is_open()) {
            return;
        }

        // 写入 WAV 文件头
        // WAVHeader header;
        // wavFile.write(reinterpret_cast<char*>(&header), sizeof(WAVHeader));
        savedFilePath = savePath.toStdString();
        isFirstStart = false;
    }

    // 添加显式启动流的调用
    if (RK_MPI_AI_StartStream(micChn) != 0) {
        qWarning() << "RK_MPI_AI_StartStream fail";
        return;
    }
    
    // 启用数据接收缓冲区
    if (RK_MPI_SYS_StartGetMediaBuffer(RK_ID_AI, micChn) != 0) {
        qWarning() << "RK_MPI_SYS_StartGetMediaBuffer fail";
        return;
    }

    isRecording = true;
    toggleButton->hide(); // 隐藏开始按钮
    stopButton->show();   // 显示停止按钮
    recordTimer.start();
    timer->start();

    // 启动轮询线程
    QThread* pollingThread = QThread::create([this]() {
        while (isRecording) {
            MEDIA_BUFFER mb = RK_MPI_SYS_GetMediaBuffer(RK_ID_AI, micChn, -1); // 使用无限等待
            if (mb) {
                void* pcmData = RK_MPI_MB_GetPtr(mb);
                uint32_t dataSize = RK_MPI_MB_GetSize(mb);
                
                // 添加日志以确认数据捕获
                qDebug() << "audio datasize:---------" << dataSize;
                
                // 写入 PCM 数据到文件
                wavFile.write(reinterpret_cast<char*>(pcmData), static_cast<std::streamsize>(dataSize));
                totalDataSize += dataSize;
                
                // 释放缓冲区
                RK_MPI_MB_ReleaseBuffer(mb);
            } else {
                qWarning() << "RK_MPI_SYS_GetMediaBuffer fail";
            }
        }
    });

    connect(pollingThread, &QThread::finished, pollingThread, &QThread::deleteLater);
    pollingThread->start();
}

void RecordingDialog::stopRecording() {
    if (isRecording) {
        isRecording = false;
        RK_MPI_SYS_StopGetMediaBuffer(RK_ID_AI, micChn);
        if (wavFile.is_open()) {
            wavFile.close();
        }


        qDebug() << "filepath:" << QString::fromStdString(savedFilePath);
        QMessageBox::information(this, "saved", "saved path:\n" + QString::fromStdString(savedFilePath));

        emit recordingSaved(QString::fromStdString(savedFilePath));

        // 重置成员变量
        isFirstStart = true;
        totalDataSize = 0;
        savedFilePath.clear();

        // 更新 UI
        stopButton->hide();
        toggleButton->show();
        timer->stop();
        timeLabel->setText("00:00.00");
    }
    accept();
}

void RecordingDialog::updateTimer() {
    if (isRecording) {
        // 计算已经经过的毫秒数
        qint64 currentElapsed = recordTimer.elapsed();
        
        // 转换为分钟:秒.毫秒格式
        int minutes = static_cast<int>(currentElapsed / 60000);
        int seconds = static_cast<int>((currentElapsed % 60000) / 1000);
        int milliseconds = static_cast<int>((currentElapsed % 1000) / 10); // 取百分之一秒
        
        // 更新UI
        QString timeText = QString("%1:%2.%3")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(seconds, 2, 10, QChar('0'))
            .arg(milliseconds, 2, 10, QChar('0'));
        
        timeLabel->setText(timeText);
    }
}

