#ifndef DETECTIONLISTPAGE_H
#define DETECTIONLISTPAGE_H

#include <QWidget>
#include "../widgets/CardWidget.h"

class DetectionListPage : public QWidget
{
    Q_OBJECT
public:
    explicit DetectionListPage(QWidget* parent = nullptr);

signals:
    void cardClicked(int index);

private:
    void setupCards();
};

#endif // DETECTIONLISTPAGE_H
