#include "DeviceManager.h"
#include "Device.h"

// 构造函数
DeviceManager::DeviceManager() = default;

// 添加设备
// MIPI 摄像头（无 RTSP）
void DeviceManager::addDevice(const QString& name) {
    m_devices.append(Device(name, Device::DeviceType::MIPI, ""));
}

// IP 摄像头（有 RTSP）
void DeviceManager::addDevice(const QString& name, const QString& rtspUrl) {
    m_devices.append(Device(name, Device::DeviceType::IP, rtspUrl));
}


// 获取设备列表
const QVector<Device>& DeviceManager::devices() const {
    return m_devices;
}

// 根据设备名称获取设备
Device* DeviceManager::getDeviceByName(const QString& name) {
    for (auto& device : m_devices) {
        if (device.name() == name) {
            return &device;
        }
    }
    return nullptr;
}
