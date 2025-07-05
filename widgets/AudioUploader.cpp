#include "AudioUploader.h"

// 构造函数
AudioUploader::AudioUploader(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);  // 初始化网络访问管理器
}

// 上传音频文件
void AudioUploader::uploadAudio(const QString &filePath, const QUrl &url)
{
    // 检查文件是否存在
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件:" << filePath;
        delete file;
        return;
    }

    // 创建HTTP多部分请求
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 添加音频文件部分
    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"audio\"; filename=\"" + file->fileName() + "\""));
    audioPart.setBodyDevice(file);
    file->setParent(multiPart);  // 文件对象由multiPart管理，确保在请求完成后自动释放

    multiPart->append(audioPart);

    // 创建网络请求
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Qt Audio Uploader");

    // 发送POST请求
    QNetworkReply *reply = manager->post(request, multiPart);
    multiPart->setParent(reply);  // multiPart由reply管理，确保在请求完成后自动释放

    // 连接信号槽，处理响应
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleUploadFinished(reply);
    });
}

void AudioUploader::handleUploadFinished(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        emit uploadFinished(true, QString(responseData));  // 上传成功
    } else {
        emit uploadFinished(false, reply->errorString());  // 上传失败
    }
    reply->deleteLater();
}
