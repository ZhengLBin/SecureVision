#ifndef MONITORWIDGET_H
#define MONITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QString>

class MonitorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MonitorWidget(const QString& iconPath, const QString& title, QWidget* parent = nullptr);

private:
    QLabel* logoLabel;
    QLabel* titleLabel;
    QString streamUrl;

protected:
    void mousePressEvent(QMouseEvent* event) override;

signals:
    void monitorClicked(const QString& rtspUrl);
};

#endif // MONITORWIDGET_H
