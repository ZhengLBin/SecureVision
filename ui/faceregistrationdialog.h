// ui/FaceRegistrationDialog.h
#ifndef FACEREGISTRATIONDIALOG_H
#define FACEREGISTRATIONDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QTimer>
#include "facerecognitionmanager.h"

class FaceRegistrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FaceRegistrationDialog(FaceRecognitionManager* faceManager, QWidget *parent = nullptr);

    void setLiveImage(const QImage& image);  // 设置实时图像用于注册

signals:
    void faceRegistered(const QString& name);

private slots:
    void onBrowseImageClicked();
    void onCaptureFromLiveClicked();
    void onRegisterClicked();
    void onCancelClicked();
    void updatePreview();

private:
    void setupUI();
    void resetDialog();
    bool validateInput();

    // UI组件
    QLineEdit* m_nameEdit;
    QLabel* m_imageLabel;
    QPushButton* m_browseBtn;
    QPushButton* m_captureBtn;
    QPushButton* m_registerBtn;
    QPushButton* m_cancelBtn;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;

    // 数据
    FaceRecognitionManager* m_faceManager;
    QImage m_selectedImage;
    QImage m_liveImage;

    // 常量
    static const int PREVIEW_SIZE = 200;
};

#endif // FACEREGISTRATIONDIALOG_H
