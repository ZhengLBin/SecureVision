#include "AudioDetectionPage.h"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
#include <QFile>

#include <thread>
#include <chrono>

#define AUDIO_PATH "default"
const QUrl AUDIO_DETECTION_URL("http://192.168.16.225:5000/detect");

AudioDetectionPage::AudioDetectionPage(QWidget* parent)
    : QWidget(parent), m_waveformManager(new WaveformManager(this))
{
    // 初始化样式
    initStyles();

    // 主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 内容区域
    contentWidget = new QWidget(this);
    contentWidget->setStyleSheet("background-color: #303338;");
    mainLayout->addWidget(contentWidget);

    // 内容布局
    contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // 初始化上下部分
    initUpperSection();
    initLowerSection();

    m_waveformManager->setupPlot(waveformDisplay);

    connect(m_waveformManager, &WaveformManager::waveformUpdated, [this]() {
        waveformDisplay->show();  // 确保波形区域可见
    });

    // 返回按钮
    backButton = new QPushButton(u8"返回篡改检测页", this);
    mainLayout->addWidget(backButton);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        emit returnRequested();  // 请求返回
    });
}

void AudioDetectionPage::resetWaveformDisplay() {
    waveformDisplay->clear();  // 清除波形图
    waveformDisplay->setVisible(false);  // 隐藏波形图
    iconLabelBtn->show();  // 显示图标
    tipLabel->show();  // 显示提示文字
    playButton->setVisible(false);  // 隐藏播放按钮
    m_waveformManager->reset();  // 重置波形管理器
}

void AudioDetectionPage::initStyles()
{
    setStyleSheet(R"(
        background-color: white;
        QLabel { color: white; }
        QPushButton {
            color: white;
            background-color: #409EFF;
            border-radius: 4px;
            padding: 8px 16px;
        }
    )");
}

void AudioDetectionPage::initUpperSection()
{
    QWidget* upperPart = new QWidget();
    QVBoxLayout* upperLayout = new QVBoxLayout(upperPart);
    upperLayout->setAlignment(Qt::AlignCenter);

    // 图标（初始可见）
    iconLabelBtn = new QPushButton();
    iconLabelBtn->setIcon(QIcon(":/AVDeepfake/micro"));
    iconLabelBtn->setCursor(Qt::PointingHandCursor);
    iconLabelBtn->setIconSize(QSize(64, 64));
    upperLayout->addWidget(iconLabelBtn, 0, Qt::AlignCenter);

    // 提示文字（初始可见）
    tipLabel = new QLabel(u8"点击下方【录制】或【本地上传】后点击【检测】验证策略效果");
    tipLabel->setStyleSheet("font-size: 24px; margin-top: 10px;color: white;");
    tipLabel->setAlignment(Qt::AlignCenter);
    tipLabel->setVisible(true); // 默认显示
    upperLayout->addWidget(tipLabel);

    // 波形展示区（初始隐藏）
    waveformDisplay = new QLabel(this);
    waveformDisplay->setStyleSheet("border-radius: 8px;");
    waveformDisplay->setAlignment(Qt::AlignCenter);
    waveformDisplay->setFixedHeight(600);
    waveformDisplay->setFixedWidth(1900);
    waveformDisplay->setVisible(false); // 初始隐藏
    upperLayout->addWidget(waveformDisplay);


    // 播放控制按钮（初始隐藏）
    QWidget* playControl = new QWidget();
    QHBoxLayout* playLayout = new QHBoxLayout(playControl);
    playLayout->setContentsMargins(0, 20, 0, 0);

    playButton = new QPushButton(u8"播放");
    playButton->setFixedSize(200, 66);
    playButton->setStyleSheet("background-color: #2468F2; color: white;");
    playButton->setVisible(false);
    connect(playButton, &QPushButton::clicked, this, &AudioDetectionPage::togglePlayback);

    playLayout->addWidget(playButton, 0, Qt::AlignCenter);
    upperLayout->addWidget(playControl);

    contentLayout->addWidget(upperPart);
}

void AudioDetectionPage::initLowerSection()
{
    QWidget* lowerPart = new QWidget();
    lowerPart->setStyleSheet("background-color: black;");
    lowerPart->setFixedHeight(150);
    QHBoxLayout* lowerLayout = new QHBoxLayout(lowerPart);
    lowerLayout->setContentsMargins(0, 0, 0, 0);

    // 格式说明
    formatLabel = new QLabel(
        u8"仅限效果测试，支持MP3、WAV、AAC、AMR、M4A格式，小于10MB、5分钟的音频"
    );
    formatLabel->setStyleSheet("font-size: 20px;color: white;");
    lowerLayout->addWidget(formatLabel, 0, Qt::AlignLeft);

    // 按钮组
    QWidget* buttonGroup = new QWidget();
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonGroup);
    buttonLayout->setSpacing(10);

    detectButton = new QPushButton(u8"检测");
    recordButton = new QPushButton(u8"录制");
    uploadButton = new QPushButton(u8"上传");

    for (auto btn : { detectButton, recordButton, uploadButton }) {
        btn->setFixedSize(140, 50);
        btn->setStyleSheet("color: white;background-color:#2468F2;font-size: 25px;");
        btn->setCursor(Qt::PointingHandCursor);

    }

    connect(recordButton, &QPushButton::clicked, this, &AudioDetectionPage::handleRecord);
    connect(uploadButton, &QPushButton::clicked, this, &AudioDetectionPage::handleFileUpload);
    connect(detectButton, &QPushButton::clicked, this, &AudioDetectionPage::handleAudioDetection);


    buttonLayout->addWidget(detectButton);
    buttonLayout->addWidget(recordButton);
    buttonLayout->addWidget(uploadButton);
    lowerLayout->addWidget(buttonGroup, 0, Qt::AlignRight | Qt::AlignCenter);

    contentLayout->addWidget(lowerPart);
}

void AudioDetectionPage::handleRecord() {
    resetWaveformDisplay();
    recordingDialog = new RecordingDialog(this);

    // 获取主窗口的左上角全局坐标
    QPoint topLeft = mapToGlobal(QPoint(300, 100));

    // 将对话框移动到主窗口的左上角
    recordingDialog->move(topLeft);

    connect(recordingDialog, &RecordingDialog::recordingSaved,
        this, &AudioDetectionPage::onRecordingSaved);

    recordingDialog->exec();
}

void AudioDetectionPage::handleFileUpload()
{
    qDebug() << "[Debug] Entering handleFileUpload";

    // 创建自定义文件选择窗口
    QWidget* fileDialog = new QWidget(this, Qt::Dialog);
    fileDialog->setWindowTitle("Select Audio File");

    // 限制窗口大小（根据屏幕分辨率 1280x720 调整）
    fileDialog->setFixedSize(800, 600);  // 设置窗口大小为 800x600

    // 获取主窗口的左上角全局坐标
    QPoint topLeft = mapToGlobal(QPoint(0, 0));

    // 将对话框移动到主窗口的左上角
    fileDialog->move(topLeft);

    // 布局和控件初始化
    QVBoxLayout* mainLayout = new QVBoxLayout(fileDialog);

    // 路径显示
    QLabel* pathLabel = new QLabel("Current Path: /");
    QLineEdit* pathEdit = new QLineEdit("/demo/.local/share/avDeepfake/recordings/");

    // 文件列表
    QListWidget* fileList = new QListWidget();
    populateFileList(fileList, "/demo/.local/share/avDeepfake/recordings/");  // 初始化根目录

    // 按钮区域
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");
    QPushButton* nextButton = new QPushButton("Next");  // Next 按钮
    QPushButton* backButton = new QPushButton("Back");  // Back 按钮

    // 初始禁用 Next 按钮
    nextButton->setEnabled(false);

    // 布局
    QHBoxLayout* pathLayout = new QHBoxLayout();
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(pathEdit);

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(nextButton);  // 将 Next 按钮添加到布局
    buttonLayout->addWidget(backButton);  // 将 Back 按钮添加到布局

    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(fileList);
    mainLayout->addLayout(buttonLayout);

    // 信号连接
    connect(pathEdit, &QLineEdit::returnPressed, [=](){
        populateFileList(fileList, pathEdit->text());
    });

    // 双击进入目录
    connect(fileList, &QListWidget::itemDoubleClicked, [=](QListWidgetItem* item){
        QFileInfo fileInfo(item->data(Qt::UserRole).toString());
        if(fileInfo.isDir()) {
            pathEdit->setText(fileInfo.filePath());
            populateFileList(fileList, fileInfo.filePath());
        }
    });

    // 选中项变化时更新 Next 按钮状态
    connect(fileList, &QListWidget::itemSelectionChanged, [=](){
        QListWidgetItem* item = fileList->currentItem();
        if (item) {
            QFileInfo fileInfo(item->data(Qt::UserRole).toString());
            nextButton->setEnabled(fileInfo.isDir());  // 如果是目录，启用 Next 按钮
        } else {
            nextButton->setEnabled(false);  // 如果没有选中项，禁用 Next 按钮
        }
    });

    // Next 按钮点击事件
    connect(nextButton, &QPushButton::clicked, [=](){
        QListWidgetItem* item = fileList->currentItem();
        if (item) {
            QFileInfo fileInfo(item->data(Qt::UserRole).toString());
            if (fileInfo.isDir()) {
                pathEdit->setText(fileInfo.filePath());
                populateFileList(fileList, fileInfo.filePath());
            } else {
                qDebug() << "[Debug] Selected item is not a directory";
            }
        } else {
            qDebug() << "[Debug] No item selected";
        }
    });

    // Back 按钮点击事件
    connect(backButton, &QPushButton::clicked, [=](){
        QDir currentDir(pathEdit->text());
        if (currentDir.cdUp()) {  // 返回上一级目录
            pathEdit->setText(currentDir.path());
            populateFileList(fileList, currentDir.path());
        } else {
            qDebug() << "[Debug] Already at the root directory";
        }
    });

    // OK 按钮点击事件
    connect(okButton, &QPushButton::clicked, [=](){
        QListWidgetItem* item = fileList->currentItem();
        if(item) {
            currentAudioPath = item->data(Qt::UserRole).toString();
            qDebug() << "[Debug] Selected file:" << currentAudioPath;

            iconLabelBtn->hide();
            tipLabel->hide();
            waveformDisplay->show();
            playButton->show();

            fileDialog->close();

            m_waveformManager->loadAndDrawWaveform(currentAudioPath);
        }
    });

    // Cancel 按钮点击事件
    connect(cancelButton, &QPushButton::clicked, fileDialog, &QWidget::close);

    // 非模态显示
    fileDialog->show();
    qDebug() << "[Debug] Custom file dialog created";
}

void AudioDetectionPage::handleAudioDetection() {
    qDebug() << "[Debug] Entering handleAudioDetection";

    if (currentAudioPath.isEmpty()) {
        qDebug() << "[Debug] No audio file selected";
        QMessageBox::warning(this, u8"警告", u8"请先选择或录制音频文件");
        return;
    }

    // 显示检测中的弹窗
    QMessageBox::information(this, u8"检测中", u8"正在检测......");

    // 调用转换函数
    wavFilePath = currentAudioPath;
    wavFilePath.replace(".pcm", ".wav");

    if (!convertPcmToWav()) {
        qDebug() << "[Debug] Failed to convert audio to WAV format";
        QMessageBox::warning(this, u8"警告", u8"音频转换失败");
        return;
    }

    // 上传音频文件
    uploader.uploadAudio(wavFilePath, AUDIO_DETECTION_URL);

    // 连接上传完成信号
    connect(&uploader, &AudioUploader::uploadFinished, this, &AudioDetectionPage::onUploadFinished);
}

void AudioDetectionPage::onUploadFinished(bool success, const QString &message) {
    if (success) {
        qDebug() << "上传成功:" << message;
    } else {
        qWarning() << "上传失败:" << message;
    }
}

bool AudioDetectionPage::convertPcmToWav()
{
    QStringList arguments;
    arguments << "-f" << "s16le"          // PCM 格式为 16 位小端（与 RK_SAMPLE_FMT_S16 匹配）
            << "-ar" << "16000"        // 采样率为 16000 Hz（与 u32SampleRate 匹配）
            << "-ac" << "2"            // 声道数为 2（立体声，与 u32Channels 匹配）
            << "-i" << currentAudioPath     // 输入 PCM 文件
            << wavFilePath;            // 输出 WAV 文件

    qDebug() << "ffmpeg arguments" << arguments;
    // 创建QProcess对象
    QProcess ffmpegProcess;
    ffmpegProcess.start("ffmpeg", arguments);  // 启动FFmpeg进程

    // 等待进程完成
    if (!ffmpegProcess.waitForFinished()) {
        qWarning() << "ffmpegProcess.waitForFinished fail" << ffmpegProcess.errorString();
        return false;
    }

    // 检查输出文件是否存在
    if (!QFile::exists(wavFilePath)) {
        qWarning() << "file not exist" << wavFilePath;
        return false;
    }

    qDebug() << "pcm to wav success" << wavFilePath;
    return true;
}

void AudioDetectionPage::populateFileList(QListWidget* listWidget, const QString& path)
{
    qDebug() << "[Debug] Loading directory:" << path;
    listWidget->clear();

    QDir dir(path);
    QFileInfoList entries = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

    // 定义允许的音频文件扩展名
    QStringList audioExtensions = {"mp3", "wav", "aac", "amr", "m4a", "pcm"};

    foreach (const QFileInfo& entry, entries) {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(entry.fileName());
        item->setData(Qt::UserRole, entry.absoluteFilePath());

        if (entry.isDir()) {
            item->setIcon(QIcon::fromTheme("folder"));
        } else {
            // 检查文件扩展名是否在允许的列表中
            if (audioExtensions.contains(entry.suffix().toLower())) {
                item->setIcon(QIcon::fromTheme("audio-file"));
            } else {
                delete item;
                continue;
            }
        }
        listWidget->addItem(item);
    }
}


void AudioDetectionPage::onRecordingSaved(const QString& filePath)
{
    currentAudioPath = filePath;
    qDebug() << "recording file is save in ：" << filePath;

    // 关闭录音对话框
    if (recordingDialog) {
        recordingDialog->close();
        recordingDialog->deleteLater();
        recordingDialog = nullptr;
    }

    // 切换界面元素
    iconLabelBtn->hide();
    tipLabel->hide();
    waveformDisplay->show();
    playButton->show();

    // 加载并绘制波形
    m_waveformManager->loadAndDrawWaveform(filePath);
}

// Fix for togglePlayback method using C++11 compatible approach

void AudioDetectionPage::togglePlayback()
{
    if (!isPlaying) {
        // Start playback
        m_shouldStop.store(false);
        isPlaying = true;
        playButton->setText(u8"停止");

        // Create global state pointers

        // Start playback thread - using this and locally created pointers
        std::thread playbackThread([this]() {
            FILE *file = nullptr;
            bool isInitialized = false;
            AO_CHN_ATTR_S ao_attr;

            // Resource cleanup function
            auto cleanupResources = [&]() {
                if (file) {
                    fclose(file);
                    file = nullptr;
                }

                if (isInitialized) {
                    RK_MPI_AO_DisableChn(0);
                    isInitialized = false;
                }

                // Update button state in UI thread
                QMetaObject::invokeMethod(this, [this]() {
                    isPlaying = false;
                    playButton->setText(u8"播放");
                }, Qt::QueuedConnection);


                qDebug() << "Playback resources cleaned up";
            };

            // Initialize audio system
            // RK_MPI_SYS_Init();

            // Set audio channel attributes
            ao_attr.pcAudioNode = const_cast<RK_CHAR*>(AUDIO_PATH);
            ao_attr.u32SampleRate = 16000;
            ao_attr.u32NbSamples = 1024;
            ao_attr.u32Channels = 2;
            ao_attr.enSampleFormat = RK_SAMPLE_FMT_S16;

            // Enable audio channel
            RK_S32 ret = RK_MPI_AO_SetChnAttr(0, &ao_attr);
            ret |= RK_MPI_AO_EnableChn(0);
            if (ret) {
                qDebug() << "ERROR: create ao[0] failed! ret=" << ret;
                cleanupResources();
                return;
            }

            // Set volume to maximum
            ret = RK_MPI_AO_SetVolume(0, 100);
            if (ret) {
                qDebug() << "ERROR: Set volume failed! ret=" << ret;
            }

            isInitialized = true;

            // Open PCM file
            file = fopen(currentAudioPath.toStdString().c_str(), "rb");
            if (!file) {
                qDebug() << "ERROR: open" << currentAudioPath << "failed!";
                cleanupResources();
                return;
            }

            // Start playback from beginning of file
            fseek(file, 0, SEEK_SET);
            qDebug() << "Starting playback from beginning of file";

            // Calculate frame time interval (milliseconds)
            double frameDurationMs = (double)ao_attr.u32NbSamples / ao_attr.u32SampleRate * 1000.0;
            qDebug() << "Frame duration:" << frameDurationMs << "ms";

            // Read audio data and play
            MEDIA_BUFFER mb = nullptr;
            RK_U32 u32ReadSize;
            MB_AUDIO_INFO_S stSampleInfo = {ao_attr.u32Channels, ao_attr.u32SampleRate,
                                           ao_attr.u32NbSamples, ao_attr.enSampleFormat};

            // Use pointer to check stop flag
            while (!m_shouldStop.load()) {
                // Create audio buffer
                mb = RK_MPI_MB_CreateAudioBufferExt(&stSampleInfo, RK_FALSE, 0);
                if (!mb) {
                    qDebug() << "ERROR: create audio buffer";
                    break;
                }

                // Get buffer size
                u32ReadSize = RK_MPI_MB_GetSize(mb);

                // Read PCM data
                size_t bytesRead = fread(RK_MPI_MB_GetPtr(mb), 1, u32ReadSize, file);
                if (bytesRead < u32ReadSize) {
                    if (bytesRead == 0) {
                        qDebug() << "End of file reached";
                        RK_MPI_MB_ReleaseBuffer(mb);
                        mb = nullptr;
                        break; // End of file, exit loop
                    }
                    // Partial data read success
                    qDebug() << "Partial read:" << bytesRead << "bytes";
                }

                // Send audio data to audio output channel
                ret = RK_MPI_SYS_SendMediaBuffer(RK_ID_AO, 0, mb);
                if (ret) {
                    qDebug() << "ERROR: RK_MPI_SYS_SendMediaBuffer failed! ret =" << ret;
                    RK_MPI_MB_ReleaseBuffer(mb);
                    mb = nullptr;
                    break;
                }

                // Use more precise delay method
                std::this_thread::sleep_for(std::chrono::milliseconds((int)frameDurationMs));

                // Release audio buffer
                RK_MPI_MB_ReleaseBuffer(mb);
                mb = nullptr;
            }

            // Release last audio buffer (if any)
            if (mb) {
                RK_MPI_MB_ReleaseBuffer(mb);
            }

            qDebug() << "Playback thread ending";
            cleanupResources();
        });

        // Detach thread to allow it to run in background
        playbackThread.detach();
    } else {
        m_shouldStop.store(true);
        isPlaying = false;
        playButton->setText(u8"播放");
        qDebug() << "Stop requested";
    }
}

