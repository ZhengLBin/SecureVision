// ai/FaceRecognitionManager.cpp
#include "facerecognitionmanager.h"
#include <QDebug>
#include <QDir>
#include <QTime>
#include <QCoreApplication>
#include <rockx.h>

FaceRecognitionManager::FaceRecognitionManager(QObject *parent)
    : QObject(parent)
    , m_faceDetHandle(nullptr)
    , m_faceRecognHandle(nullptr)
    , m_faceLandmarkHandle(nullptr)
    , m_initialized(false)
    , m_detectionThreshold(0.5f)
    , m_recognitionThreshold(0.7f)
    , m_lastDetectionTime(0.0f)
    , m_lastRecognitionTime(0.0f)
    , m_detectionCount(0)
    , m_recognitionCount(0)
{
    // 初始化数据库
    m_database = new FaceDatabase(this);
    connect(m_database, &FaceDatabase::databaseError, this, &FaceRecognitionManager::onDatabaseError);
}

FaceRecognitionManager::~FaceRecognitionManager()
{
    cleanup();
}

bool FaceRecognitionManager::initialize(const QString& modelPath)
{
    QMutexLocker locker(&m_mutex);

    if (m_initialized) {
        qDebug() << "FaceRecognitionManager already initialized";
        return true;
    }

    m_modelPath = modelPath;

    // 1. 初始化数据库
    QString dbPath = QCoreApplication::applicationDirPath() + "/data/database/face_recognition.db";
    if (!m_database->initialize(dbPath)) {
        qDebug() << "Failed to initialize face database";
        emit initializationFailed("Database initialization failed");
        return false;
    }

    // 2. 初始化RockX
    if (!initializeRockX()) {
        qDebug() << "Failed to initialize RockX";
        emit initializationFailed("RockX initialization failed");
        return false;
    }

    m_initialized = true;
    qDebug() << "FaceRecognitionManager initialized successfully";
    qDebug() << "Model path:" << m_modelPath;
    qDebug() << "Registered faces:" << m_database->getTotalFaceCount();

    return true;
}

bool FaceRecognitionManager::initializeRockX()
{
    rockx_ret_t ret;

    // 设置模型数据路径
    if (!setDataPath()) {
        return false;
    }

    // 1. 初始化人脸检测
    ret = rockx_create(&m_faceDetHandle, ROCKX_MODULE_FACE_DETECTION, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "Failed to create face detection handle, error:" << ret;
        return false;
    }

    // 2. 初始化人脸识别
    ret = rockx_create(&m_faceRecognHandle, ROCKX_MODULE_FACE_RECOGNIZE, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "Failed to create face recognition handle, error:" << ret;
        cleanup();
        return false;
    }

    // 3. 初始化人脸关键点检测 (用于人脸对齐)
    ret = rockx_create(&m_faceLandmarkHandle, ROCKX_MODULE_FACE_LANDMARK_5, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "Failed to create face landmark handle, error:" << ret;
        cleanup();
        return false;
    }

    qDebug() << "RockX modules initialized successfully";
    return true;
}

bool FaceRecognitionManager::setDataPath()
{
    // 设置RockX模型数据路径 (参考指导文档)
    rockx_ret_t ret = rockx_set_data_path(m_faceDetHandle, m_modelPath.toLocal8Bit().data());
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "Failed to set RockX data path:" << m_modelPath << ", error:" << ret;
        return false;
    }

    qDebug() << "RockX data path set to:" << m_modelPath;
    return true;
}

QVector<FaceInfo> FaceRecognitionManager::detectFaces(const QImage& image)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || image.isNull()) {
        return QVector<FaceInfo>();
    }

    QTime timer;
    timer.start();

    QVector<FaceInfo> faces = processDetection(image);

    m_lastDetectionTime = timer.elapsed();
    m_detectionCount++;

    logPerformance("Face Detection", m_lastDetectionTime);

    if (!faces.isEmpty()) {
        emit faceDetected(faces);
    }

    return faces;
}

QVector<FaceInfo> FaceRecognitionManager::detectAndRecognizeFaces(const QImage& image)
{
    // 1. 首先进行人脸检测
    QVector<FaceInfo> detectedFaces = detectFaces(image);

    if (detectedFaces.isEmpty()) {
        return detectedFaces;
    }

    // 2. 对检测到的人脸进行识别
    QTime timer;
    timer.start();

    QVector<FaceInfo> recognizedFaces = processRecognition(detectedFaces, image);

    m_lastRecognitionTime = timer.elapsed();
    m_recognitionCount++;

    logPerformance("Face Recognition", m_lastRecognitionTime);

    return recognizedFaces;
}

QVector<FaceInfo> FaceRecognitionManager::processDetection(const QImage& image)
{
    QVector<FaceInfo> faces;

    // 预处理图像
    QImage processedImage = preprocessImage(image);
    rockx_image_t rockxImage = qImageToRockxImage(processedImage);

    // 执行人脸检测
    rockx_object_array_t faceArray;
    rockx_ret_t ret = rockx_face_detect(m_faceDetHandle, &rockxImage, &faceArray, nullptr);

    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "Face detection failed, error:" << ret;
        return faces;
    }

    // 解析检测结果
    for (int i = 0; i < faceArray.count; ++i) {
        const rockx_object_t& obj = faceArray.object[i];

        if (obj.score >= m_detectionThreshold) {
            FaceInfo faceInfo;
            faceInfo.bbox = QRect(obj.box.left, obj.box.top,
                                  obj.box.right - obj.box.left,
                                  obj.box.bottom - obj.box.top);
            faceInfo.confidence = obj.score;
            faceInfo.isRecognized = false;  // 仅检测，未识别

            faces.append(faceInfo);
        }
    }

    return faces;
}

QVector<FaceInfo> FaceRecognitionManager::processRecognition(const QVector<FaceInfo>& detectedFaces, const QImage& image)
{
    QVector<FaceInfo> recognizedFaces = detectedFaces;

    for (int i = 0; i < recognizedFaces.size(); ++i) {
        recognizedFaces[i] = recognizeSingleFace(recognizedFaces[i], image);
    }

    return recognizedFaces;
}

FaceInfo FaceRecognitionManager::recognizeSingleFace(const FaceInfo& detectedFace, const QImage& image)
{
    FaceInfo faceInfo = detectedFace;

    // 从原图中裁剪人脸区域
    QImage faceImage = image.copy(detectedFace.bbox);
    if (faceImage.isNull()) {
        return faceInfo;
    }

    // 提取人脸特征
    QByteArray feature = extractFaceFeature(faceImage);
    if (feature.isEmpty()) {
        return faceInfo;
    }

    // 在数据库中查找最佳匹配
    float similarity;
    int matchId = m_database->findBestMatch(feature, similarity);

    if (matchId > 0 && similarity >= m_recognitionThreshold) {
        // 找到匹配的人脸
        FaceRecord record = m_database->getFaceRecord(matchId);
        if (record.id > 0) {
            faceInfo.personName = record.name;
            faceInfo.faceId = matchId;
            faceInfo.similarity = similarity;
            faceInfo.isRecognized = true;

            // 更新数据库统计信息
            m_database->updateLastSeen(matchId);
            m_database->incrementRecognitionCount(matchId);

            emit faceRecognized(record.name, similarity);
        }
    } else {
        // 未知人脸
        emit unknownFaceDetected(detectedFace.bbox);
    }

    return faceInfo;
}

QByteArray FaceRecognitionManager::extractFaceFeature(const QImage& faceImage)
{
    // 这里应该使用RockX的人脸特征提取功能
    // 由于RockX API的具体使用方式可能需要参考更详细的文档
    // 暂时返回空，实际实现时需要调用rockx_face_recognize相关API

    // TODO: 实现实际的特征提取
    // rockx_image_t rockxImage = qImageToRockxImage(faceImage);
    // rockx_feature_t feature;
    // rockx_ret_t ret = rockx_face_feature_extract(m_faceRecognHandle, &rockxImage, &feature);

    return QByteArray();
}

bool FaceRecognitionManager::registerFace(const QString& name, const QImage& faceImage)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || name.isEmpty() || faceImage.isNull()) {
        return false;
    }

    // 检查是否已存在同名人脸
    if (m_database->faceExists(name)) {
        qDebug() << "Face with name already exists:" << name;
        return false;
    }

    // 提取人脸特征
    QByteArray feature = extractFaceFeature(faceImage);
    if (feature.isEmpty()) {
        qDebug() << "Failed to extract face feature for:" << name;
        return false;
    }

    // 保存到数据库
    QString imagePath = QString("faces/%1_%2.jpg")
                            .arg(name)
                            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    if (!m_database->addFaceRecord(name, imagePath, feature)) {
        qDebug() << "Failed to add face record to database:" << name;
        return false;
    }

    // 保存图像文件
    QString fullImagePath = QCoreApplication::applicationDirPath() + "/data/faces/" + QFileInfo(imagePath).fileName();
    QDir().mkpath(QFileInfo(fullImagePath).absolutePath());

    if (!faceImage.save(fullImagePath)) {
        qDebug() << "Failed to save face image:" << fullImagePath;
        // 即使图像保存失败，数据库记录已添加，返回成功
    }

    qDebug() << "Face registered successfully:" << name;
    return true;
}

rockx_image_t FaceRecognitionManager::qImageToRockxImage(const QImage& image)
{
    rockx_image_t rockxImage;

    // 转换为RGB格式
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

    rockxImage.width = rgbImage.width();
    rockxImage.height = rgbImage.height();
    rockxImage.pixel_format = ROCKX_PIXEL_FORMAT_RGB888;
    rockxImage.data = (uint8_t*)rgbImage.bits();
    rockxImage.size = rgbImage.sizeInBytes();

    return rockxImage;
}

QImage FaceRecognitionManager::preprocessImage(const QImage& image)
{
    // 根据需要进行图像预处理
    // 例如：调整大小、增强对比度等
    if (image.width() > 1920 || image.height() > 1080) {
        return image.scaled(1920, 1080, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
}

void FaceRecognitionManager::cleanup()
{
    if (m_faceDetHandle) {
        rockx_destroy(m_faceDetHandle);
        m_faceDetHandle = nullptr;
    }

    if (m_faceRecognHandle) {
        rockx_destroy(m_faceRecognHandle);
        m_faceRecognHandle = nullptr;
    }

    if (m_faceLandmarkHandle) {
        rockx_destroy(m_faceLandmarkHandle);
        m_faceLandmarkHandle = nullptr;
    }

    m_initialized = false;
}

QStringList FaceRecognitionManager::getAllRegisteredNames()
{
    QStringList names;
    auto records = m_database->getAllFaceRecords();

    for (const auto& record : records) {
        names.append(record.name);
    }

    return names;
}

void FaceRecognitionManager::logPerformance(const QString& operation, float timeMs)
{
    if (timeMs > 100) {  // 只记录耗时较长的操作
        qDebug() << QString("%1 took %.2f ms").arg(operation).arg(timeMs);
    }
}

void FaceRecognitionManager::onDatabaseError(const QString& error)
{
    qDebug() << "Database error in FaceRecognitionManager:" << error;
    emit databaseError(error);
}
