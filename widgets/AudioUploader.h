#ifndef AUDIOUPLOADER_H
#define AUDIOUPLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QHttpMultiPart>
#include <QDebug>

class AudioUploader : public QObject
{
    Q_OBJECT

public:
    explicit AudioUploader(QObject *parent = nullptr);  // 构造函数
    void uploadAudio(const QString &filePath, const QUrl &url);  // 上传音频文件

private slots:
    void handleUploadFinished(QNetworkReply *reply);  // 处理上传完成信号

signals:
    void uploadFinished(bool success, const QString &message);
    
private:
    QNetworkAccessManager *manager;  // 网络访问管理器
};

#endif // AUDIOUPLOADER_H
