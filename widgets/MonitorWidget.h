#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QString>

class MonitorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorWidget(const QString& iconPath, const QString& title, const int typeNum, QWidget* parent = nullptr);

private:
    QLabel* logoLabel;
    QLabel* titleLabel;
    QString m_rtspUrl;
    const int m_type;

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void monitorClicked(const QString& rtspUrl, const int m_type);
};

#endif // MONITORWIDGET_H
