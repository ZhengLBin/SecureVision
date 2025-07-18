// ui/FaceRegistrationDialog.cpp
#include "faceregistrationdialog.h"
#include <QGridLayout>
#include <QGroupBox>
#include <QPixmap>

FaceRegistrationDialog::FaceRegistrationDialog(FaceRecognitionManager* faceManager, QWidget *parent)
    : QDialog(parent)
    , m_faceManager(faceManager)
{
    setWindowTitle("人脸注册");
    setFixedSize(500, 400);
    setModal(true);

    setupUI();
    resetDialog();
}

void FaceRegistrationDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 输入区域
    QGroupBox* inputGroup = new QGroupBox("注册信息", this);
    QGridLayout* inputLayout = new QGridLayout(inputGroup);

    inputLayout->addWidget(new QLabel("姓名:"), 0, 0);
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("请输入人员姓名");
    inputLayout->addWidget(m_nameEdit, 0, 1);

    // 图像区域
    QGroupBox* imageGroup = new QGroupBox("人脸图像", this);
    QVBoxLayout* imageLayout = new QVBoxLayout(imageGroup);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setFixedSize(PREVIEW_SIZE, PREVIEW_SIZE);
    m_imageLabel->setStyleSheet("border: 2px dashed #ccc; background: #f9f9f9;");
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setText("未选择图像");
    m_imageLabel->setScaledContents(true);

    QHBoxLayout* imageBtnLayout = new QHBoxLayout();
    m_browseBtn = new QPushButton("浏览文件", this);
    m_captureBtn = new QPushButton("从实时画面捕获", this);
    imageBtnLayout->addWidget(m_browseBtn);
    imageBtnLayout->addWidget(m_captureBtn);

    imageLayout->addWidget(m_imageLabel, 0, Qt::AlignCenter);
    imageLayout->addLayout(imageBtnLayout);

    // 状态区域
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: blue;");

    // 按钮区域
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_registerBtn = new QPushButton("注册", this);
    m_cancelBtn = new QPushButton("取消", this);

    m_registerBtn->setStyleSheet("QPushButton { background: #4CAF50; color: white; padding: 8px 16px; }");
    m_cancelBtn->setStyleSheet("QPushButton { background: #f44336; color: white; padding: 8px 16px; }");

    btnLayout->addStretch();
    btnLayout->addWidget(m_registerBtn);
    btnLayout->addWidget(m_cancelBtn);

    // 主布局
    mainLayout->addWidget(inputGroup);
    mainLayout->addWidget(imageGroup);
    mainLayout->addWidget(m_progressBar);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addLayout(btnLayout);

    // 连接信号
    connect(m_browseBtn, &QPushButton::clicked, this, &FaceRegistrationDialog::onBrowseImageClicked);
    connect(m_captureBtn, &QPushButton::clicked, this, &FaceRegistrationDialog::onCaptureFromLiveClicked);
    connect(m_registerBtn, &QPushButton::clicked, this, &FaceRegistrationDialog::onRegisterClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, &FaceRegistrationDialog::onCancelClicked);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &FaceRegistrationDialog::updatePreview);
}

void FaceRegistrationDialog::onBrowseImageClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择人脸图像",
        "",
        "图像文件 (*.png *.jpg *.jpeg *.bmp)"
        );

    if (!fileName.isEmpty()) {
        m_selectedImage = QImage(fileName);
        if (!m_selectedImage.isNull()) {
            updatePreview();
            m_statusLabel->setText("已选择图像文件");
        } else {
            QMessageBox::warning(this, "错误", "无法加载选择的图像文件");
        }
    }
}

void FaceRegistrationDialog::onCaptureFromLiveClicked()
{
    if (!m_liveImage.isNull()) {
        m_selectedImage = m_liveImage;
        updatePreview();
        m_statusLabel->setText("已从实时画面捕获图像");
    } else {
        QMessageBox::information(this, "提示", "暂无实时画面可供捕获");
    }
}

void FaceRegistrationDialog::onRegisterClicked()
{
    if (!validateInput()) {
        return;
    }

    // 显示进度条
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);  // 不确定进度
    m_registerBtn->setEnabled(false);
    m_statusLabel->setText("正在注册人脸...");

    // 异步执行注册
    QTimer::singleShot(100, [this]() {
        bool success = m_faceManager->registerFace(m_nameEdit->text().trimmed(), m_selectedImage);

        m_progressBar->setVisible(false);
        m_registerBtn->setEnabled(true);

        if (success) {
            m_statusLabel->setText("人脸注册成功！");
            m_statusLabel->setStyleSheet("color: green;");

            emit faceRegistered(m_nameEdit->text().trimmed());

            QTimer::singleShot(1500, this, &QDialog::accept);
        } else {
            m_statusLabel->setText("人脸注册失败，请重试");
            m_statusLabel->setStyleSheet("color: red;");
        }
    });
}

void FaceRegistrationDialog::onCancelClicked()
{
    reject();
}

void FaceRegistrationDialog::updatePreview()
{
    if (!m_selectedImage.isNull()) {
        QPixmap pixmap = QPixmap::fromImage(m_selectedImage.scaled(
            PREVIEW_SIZE, PREVIEW_SIZE,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            ));
        m_imageLabel->setPixmap(pixmap);
    } else {
        m_imageLabel->clear();
        m_imageLabel->setText("未选择图像");
    }
}

bool FaceRegistrationDialog::validateInput()
{
    QString name = m_nameEdit->text().trimmed();

    if (name.isEmpty()) {
        QMessageBox::warning(this, "输入错误", "请输入人员姓名");
        m_nameEdit->setFocus();
        return false;
    }

    if (m_selectedImage.isNull()) {
        QMessageBox::warning(this, "输入错误", "请选择或捕获人脸图像");
        return false;
    }

    // 检查姓名是否已存在
    QStringList existingNames = m_faceManager->getAllRegisteredNames();
    if (existingNames.contains(name, Qt::CaseInsensitive)) {
        QMessageBox::warning(this, "注册失败", QString("姓名 '%1' 已存在，请使用不同的姓名").arg(name));
        m_nameEdit->setFocus();
        return false;
    }

    return true;
}

void FaceRegistrationDialog::setLiveImage(const QImage& image)
{
    m_liveImage = image;
    m_captureBtn->setEnabled(!image.isNull());
}

void FaceRegistrationDialog::resetDialog()
{
    m_nameEdit->clear();
    m_selectedImage = QImage();
    m_liveImage = QImage();
    updatePreview();
    m_progressBar->setVisible(false);
    m_registerBtn->setEnabled(true);
    m_statusLabel->setText("请输入注册信息");
    m_statusLabel->setStyleSheet("color: blue;");
}
