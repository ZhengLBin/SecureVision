#include "Device.h"

// 构造函数
Device::Device()
    : m_name(""), m_rtspUrl("") {}

Device::Device(const QString& name, DeviceType type, const QUrl& rtspUrl)
    : m_name(name), m_type(type), m_rtspUrl(rtspUrl) {}

Device::Device(const QString& name, DeviceType type, const QString& devicePath)
    : m_name(name), m_type(type), m_devicePath(devicePath) {}

// 获取设备名称
QString Device::name() const {
    return m_name;
}

// 获取设备类型
Device::DeviceType Device::type() const {
    return m_type;
}

// 获取RTSP流地址
QUrl Device::rtspUrl() const {
    return m_rtspUrl;
}

