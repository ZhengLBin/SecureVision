#include "WaveFormManager.h"
#include <QFile>
#include <QThread>
#include <QDebug>

WaveformManager::WaveformManager(QObject *parent)
    : QObject(parent), m_customPlot(nullptr), m_adecChn(0), m_isDecoding(false) {}

WaveformManager::~WaveformManager() {
    if (m_customPlot) {
        delete m_customPlot;
    }
}

void WaveformManager::setupPlot(QWidget *container) {
    m_customPlot = new QCustomPlot(container);
    QVBoxLayout *layout = new QVBoxLayout(container);
    layout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_customPlot);
    container->setLayout(layout);

    // 设置绘图区域的大小为 1900x600
    m_customPlot->setMinimumSize(1100, 300);
    m_customPlot->setMaximumSize(1100, 300);

    // 初始化绘图样式
    m_customPlot->xAxis->setLabel("");
    m_customPlot->yAxis->setLabel("");

    // 设置背景颜色
    m_customPlot->setBackground(QBrush(QColor("#303338")));

    // 设置坐标轴和文字颜色为白色
    m_customPlot->xAxis->setLabelColor(Qt::white);
    m_customPlot->yAxis->setLabelColor(Qt::white);
    m_customPlot->xAxis->setTickLabelColor(Qt::white);
    m_customPlot->yAxis->setTickLabelColor(Qt::white);
    m_customPlot->xAxis->setBasePen(QPen(Qt::white));
    m_customPlot->yAxis->setBasePen(QPen(Qt::white));
    m_customPlot->xAxis->setTickPen(QPen(Qt::white));
    m_customPlot->yAxis->setTickPen(QPen(Qt::white));
    m_customPlot->xAxis->setSubTickPen(QPen(Qt::white));
    m_customPlot->yAxis->setSubTickPen(QPen(Qt::white));

    // 设置坐标轴标签和刻度标签的字体大小为 8px
    QFont axisFont;
    axisFont.setPointSize(8);  // 设置字体大小为 8px
    m_customPlot->xAxis->setLabelFont(axisFont);
    m_customPlot->yAxis->setLabelFont(axisFont);
    m_customPlot->xAxis->setTickLabelFont(axisFont);
    m_customPlot->yAxis->setTickLabelFont(axisFont);
}

void WaveformManager::reset() {
    // 停止解码（如果正在进行）
    m_isDecoding.store(false);

    // 清除 PCM 数据
    m_pcmSamples.clear();

    // 清除绘图区域
    if (m_customPlot) {
        m_customPlot->clearGraphs();
        m_customPlot->replot();
    }

    qDebug() << "WaveformManager reset: cleared waveform data and plot.";
}


void WaveformManager::loadAndDrawWaveform(const QString &filePath) {
    m_pcmSamples.clear();
    m_isDecoding.store(true);

    // 在后台线程中解码
    QThread *decodeThread = QThread::create([this, filePath]() {
        decodeAudioFile(filePath);
        QMetaObject::invokeMethod(this, &WaveformManager::onDecodeFinished, Qt::QueuedConnection);
    });
    connect(decodeThread, &QThread::finished, decodeThread, &QObject::deleteLater);
    decodeThread->start();
}

void WaveformManager::decodeAudioFile(const QString &filePath) {
    // 检查文件扩展名
    if (filePath.endsWith(".pcm", Qt::CaseInsensitive)) {
        // 直接读取 PCM 文件
        QFile audioFile(filePath);
        if (!audioFile.open(QIODevice::ReadOnly)) {
            qDebug() << "audioFile open error" << filePath;
            return;
        }

        // 读取 PCM 数据
        QByteArray pcmData = audioFile.readAll();
        audioFile.close();

        // 将数据转换为 qint16 数组
        const qint16 *pcmSamples = reinterpret_cast<const qint16*>(pcmData.constData());
        size_t sampleCount = static_cast<size_t>(pcmData.size()) / sizeof(qint16);

        // 存储 PCM 数据
        m_pcmSamples.resize(static_cast<int>(sampleCount));
        std::copy(pcmSamples, pcmSamples + sampleCount, m_pcmSamples.begin());  // 复制数据

        // 通知解码完成
        QMetaObject::invokeMethod(this, &WaveformManager::onDecodeFinished, Qt::QueuedConnection);
    } else {
        // 如果是压缩格式（如 AAC、MP3），调用 RKMedia 解码
        // 这里可以添加对其他格式的支持
        qDebug() << "Wrong file type:" << filePath;
    }
}

void WaveformManager::onDecodeFinished() {
    if (m_pcmSamples.isEmpty()) {
        qDebug() << "无有效 PCM 数据!";
        return;
    }

    // 绘制波形图
    drawWaveform(m_pcmSamples);
    emit waveformUpdated();
}

void WaveformManager::drawWaveform(const QVector<qint16> &samples) {
    if (samples.isEmpty()) {
        qDebug() << "no valid data!";
        return;
    }

    // 清除之前的图形
    m_customPlot->clearGraphs();

    // 添加新的图形
    m_customPlot->addGraph();

    // 设置波形颜色为 #f2530a
    QPen pen;
    pen.setColor(QColor("#f2530a"));
    pen.setWidth(2);  // 设置波形线条的宽度
    m_customPlot->graph(0)->setPen(pen);

    // 采样减少数据量
    QVector<double> x, y;
    const int SAMPLE_INTERVAL = 100;
    for (int i = 0; i < samples.size(); i += SAMPLE_INTERVAL) {
        x.append(i);
        y.append(samples[i]);
    }

    // 设置数据
    m_customPlot->graph(0)->setData(x, y);

    // 设置坐标轴范围
    m_customPlot->xAxis->setRange(0, samples.size());
    m_customPlot->yAxis->setRange(-32768, 32767);

    // 重新绘制
    m_customPlot->replot();
}
