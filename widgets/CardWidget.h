#pragma once
#ifndef CARDWIDGET_H
#define CARDWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>

class CardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CardWidget(const QString& iconPath, const QString& title, const QString& description, QWidget* parent = nullptr);

signals:
    void cardClicked(int cardId); // 信号：表示卡片被点击

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    QLabel* logoLabel;
    QLabel* titleLabel;
    QLabel* descriptionLabel;
};

#endif // CARDWIDGET_H
