#ifndef DEVICE_H
#define DEVICE_H

#include <QString>
#include <QUrl>

class Device {
public:
    enum class DeviceType {
        MIPI,
        IP,
        USB

    };

    // 构造函数
    Device(); // 添加默认构造函数声明
    Device(const QString& name, DeviceType type, const QUrl& rtspUrl);
    Device(const QString& name, DeviceType type, const QString& devicePath);

    // 获取设备名称
    QString name() const;

    // 获取设备类型
    DeviceType type() const;

    // 获取RTSP流地址
    QUrl rtspUrl() const;


private:
    QString m_name;       // 设备名称
    DeviceType m_type;    // 设备类型
    QUrl m_rtspUrl;    // RTSP流地址
    QString m_devicePath;
};

#endif // DEVICE_H
