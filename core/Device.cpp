#include "Device.h"

// 构造函数
Device::Device()
    : m_name(""), m_rtspUrl("") {}

Device::Device(const QString& name, DeviceType type, const QString& rtspUrl)
    : m_name(name), m_type(type), m_rtspUrl(rtspUrl) {}

// 获取设备名称
QString Device::name() const {
    return m_name;
}

// 获取设备类型
Device::DeviceType Device::type() const {
    return m_type;
}

// 获取RTSP流地址
QString Device::rtspUrl() const {
    return m_rtspUrl;
}

// 设置ONVIF信息
void Device::setOnvifInfo(const QString& onvifUrl) {
    m_onvifUrl = onvifUrl;
}

// 获取ONVIF信息
QString Device::onvifInfo() const {
    return m_onvifUrl;
}
