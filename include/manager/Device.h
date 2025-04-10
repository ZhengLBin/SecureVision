#ifndef DEVICE_H
#define DEVICE_H

#include <QString>

class Device {
public:
    enum class DeviceType {
        MIPI,  // MIPI 摄像头
        IP     // IP 摄像头
    };

    // 构造函数
    Device(); // 添加默认构造函数声明
    Device(const QString& name, DeviceType type, const QString& rtspUrl);

    // 获取设备名称
    QString name() const;

    // 获取设备类型
    DeviceType type() const;

    // 获取RTSP流地址
    QString rtspUrl() const;

    // 设置和获取设备的ONVIF信息
    void setOnvifInfo(const QString& onvifUrl);
    QString onvifInfo() const;

private:
    QString m_name;       // 设备名称
    DeviceType m_type;    // 设备类型
    QString m_rtspUrl;    // RTSP流地址
    QString m_onvifUrl;   // 用于存储通过ONVIF获取的IP摄像头信息
};

#endif // DEVICE_H
