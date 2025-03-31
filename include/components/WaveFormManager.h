#ifndef WAVEFORMMANAGER_H
#define WAVEFORMMANAGER_H

#include <QObject>
#include <QVector>
#include "QcustomPlot.h"
#include <rkmedia/rkmedia_api.h>

class WaveformManager : public QObject {
    Q_OBJECT

public:
    explicit WaveformManager(QObject *parent = nullptr);
    ~WaveformManager();

    // 初始化绘图控件
    void setupPlot(QWidget *container);
    // 加载并绘制波形
    void loadAndDrawWaveform(const QString &filePath);
    void reset();

signals:
    void waveformUpdated();  // 波形更新信号

private slots:
    void onDecodeFinished();

private:
    void decodeAudioFile(const QString &filePath);
    void drawWaveform(const QVector<qint16> &samples);

    QCustomPlot *m_customPlot;      // QCustomPlot对象
    QVector<qint16> m_pcmSamples;   // PCM数据
    int m_adecChn;                  // 解码通道号
    std::atomic<bool> m_isDecoding; // 解码状态标志
};

#endif // WAVEFORMMANAGER_H
