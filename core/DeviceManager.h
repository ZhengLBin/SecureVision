#ifndef DEVICEMANAGER_H
#define DEVICEMANAGER_H

#include "Device.h"
#include <QVector>

class DeviceManager {
public:
    DeviceManager();

    // 添加设备
    void addDevice(const QString& name);
    void addDevice(const QString& name, const QString& rtspUrl);

    // 获取设备列表
    const QVector<Device>& devices() const;

    // 根据设备名称获取设备
    Device* getDeviceByName(const QString& name);

private:
    QVector<Device> m_devices;  // 存储设备的容器
};

#endif // DEVICEMANAGER_H
