#ifndef MONITORLISTPAGE_H
#define MONITORLISTPAGE_H

#include <QWidget>
#include <QVector>
#include "../components/MonitorWidget.h"
#include "../manager/DeviceManager.h"

class MonitorListPage : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorListPage(QWidget* parent = nullptr);

signals:
    void openShowMonitorPage(const QString& rtspUrl, const int m_type);

private:
    void setupCards();
    DeviceManager m_deviceManager;
    static const QString cardLogoPath;
};

#endif // MONITORLISTPAGE_H
