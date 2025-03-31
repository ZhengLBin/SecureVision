#ifndef MONITORLISTPAGE_H
#define MONITORLISTPAGE_H

#include <QWidget>
#include <QVector>
#include "../components/MonitorWidget.h"

class MonitorListPage : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorListPage(QWidget* parent = nullptr);

signals:
    void openShowMonitorPage(const QString& rtspUrl);

private:
    void setupCards();
    QVector<QString> m_rtspStreams;
};

#endif // MONITORLISTPAGE_H