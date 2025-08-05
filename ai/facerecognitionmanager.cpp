// ai/FaceRecognitionManager.cpp
#include "facerecognitionmanager.h"
#include <QTime>
#include <QCoreApplication>
#include <QDir>

FaceRecognitionManager::FaceRecognitionManager(QObject *parent)
    : QObject(parent)
    , m_faceDetHandle(nullptr)
    , m_faceLandmarkHandle(nullptr)
    , m_faceRecognizeHandle(nullptr)
    , m_initialized(false)
    , m_detectionThreshold(0.5f)
    , m_recognitionThreshold(0.7f)
    , m_lastDetectionTime(0.0f)
    , m_lastRecognitionTime(0.0f)
    , m_detectionCount(0)
    , m_recognitionCount(0)
{
    qDebug() << "FaceRecognitionManager: Constructor started";

    // 初始化数据库
    m_database = new FaceDatabase(this);

    qDebug() << "FaceRecognitionManager: Constructor completed";
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

    // Define possible model paths on target device (ordered by priority)
    QStringList possiblePaths;

    // 1. First try user-specified path
    if (!modelPath.isEmpty()) {
        possiblePaths << modelPath;
    }

    // 2. Try environment variable
    QString envPath = QString::fromLocal8Bit(qgetenv("ROCKX_MODEL_PATH"));
    if (!envPath.isEmpty()) {
        possiblePaths << envPath;
    }

    // 3. Standard installation paths (where CMake would install them)
    possiblePaths << "/usr/local/share/rockx_data"      // Primary CMake install location
                  << "/share/rockx_data"                // Alternate CMake install location
                  << "/opt/rockx/data"                  // Possible RockX standard path
                  << "/usr/share/rockx/data"            // System standard path
                  << "/usr/share/rockx_data"            // Variant path
                  << "/oem/usr/share/rockx_data";       // Common OEM partition path for RV1126

    // 4. Application-relative paths
    QString appDir = QCoreApplication::applicationDirPath();
    possiblePaths << appDir + "/rockx_data"
                  << appDir + "/../share/rockx_data"
                  << appDir + "/../../share/rockx_data";

    // Find first valid path
    QString validPath;
    qDebug() << "========== Searching for RockX model files ==========";

    for (const QString& path : possiblePaths) {
        qDebug() << "Checking path:" << path;

        QDir dir(path);
        if (!dir.exists()) {
            qDebug() << "  ✗ Directory does not exist";
            continue;
        }

        if (validateModelFiles(path)) {
            validPath = path;
            qDebug() << "  ✓ Found valid model path:" << validPath;
            break;
        } else {
            qDebug() << "  ✗ Directory exists but model files are missing";
        }
    }

    if (validPath.isEmpty()) {
        qDebug() << " No valid RockX model path found!";
        qDebug() << "All searched paths:";
        for (const QString& path : possiblePaths) {
            qDebug() << "  - " << path;
        }
        qDebug() << "";
        qDebug() << "Possible solutions:";
        qDebug() << "1. Ensure model files are properly installed on target device";
        qDebug() << "2. Set environment variable: export ROCKX_MODEL_PATH=/path/to/models";
        qDebug() << "3. Verify file permissions are correct";

        emit initializationFailed("No valid RockX model path found");
        return false;
    }

    m_modelPath = validPath;

    qDebug() << "========== FaceRecognitionManager Initialization Started ==========";
    qDebug() << "✓ Final model path:" << m_modelPath;
    qDebug() << "✓ Current user:" << qgetenv("USER");
    qDebug() << "✓ Application path:" << QCoreApplication::applicationDirPath();
    qDebug() << "✓ Current working directory:" << QDir::currentPath();

    // 2. Initialize database - 使用用户主目录避免权限问题
    QString appDataDir;

    // 尝试不同的数据目录位置
    QStringList possibleDataDirs = {
        QDir::homePath() + "/.local/share/SecureVision",        // 用户主目录
        "/tmp/SecureVision",                                    // 临时目录
        QCoreApplication::applicationDirPath() + "/data",       // 应用程序目录
        "/var/tmp/SecureVision"                                 // 系统临时目录
    };

    for (const QString& dir : possibleDataDirs) {
        QDir testDir(dir);
        if (testDir.exists() || testDir.mkpath(".")) {
            // 测试写权限
            QString testFile = dir + "/test_write.tmp";
            QFile file(testFile);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
                file.remove();
                appDataDir = dir;
                qDebug() << "✓ Selected data directory:" << appDataDir;
                break;
            }
        }
    }

    if (appDataDir.isEmpty()) {
        qDebug() << " Failed to find writable data directory";
        emit initializationFailed("No writable data directory found");
        return false;
    }

    // 确保数据库目录存在
    QDir().mkpath(appDataDir + "/database");

    QString dbPath = appDataDir + "/database/face_recognition.db";
    qDebug() << "Database path:" << dbPath;

    if (!m_database->initialize(dbPath)) {
        qDebug() << " Database initialization failed";
        emit initializationFailed("Database initialization failed");
        return false;
    }
    qDebug() << "✓ Database initialized successfully";

    // 3. Initialize RockX
    if (!initializeRockX()) {
        qDebug() << " RockX initialization failed";
        emit initializationFailed("RockX initialization failed");
        return false;
    }
    qDebug() << "✓ RockX initialized successfully";

    m_initialized = true;
    qDebug() << "========== FaceRecognitionManager Initialization Completed ==========";
    qDebug() << "  ✓ Model path:" << m_modelPath;
    qDebug() << "  ✓ Detection threshold:" << m_detectionThreshold;
    qDebug() << "  ✓ Recognition threshold:" << m_recognitionThreshold;
    qDebug() << "  ✓ Registered faces:" << m_database->getTotalFaceCount();

    return true;
}

bool FaceRecognitionManager::validateModelFiles(const QString& modelPath)
{
    qDebug() << "========== Validating Model Files ==========";
    qDebug() << "Model path:" << modelPath;

    // Check if directory exists
    QDir modelDir(modelPath);
    if (!modelDir.exists()) {
        qDebug() << " Model directory does not exist:" << modelPath;
        return false;
    }
    qDebug() << "✓ Model directory exists";

    // Check required model files
    QStringList requiredFiles = {
        "face_detection_v3_fast.data",
        "face_landmark5.data",
        "face_recognition.data"
    };

    qDebug() << "Checking required model files:";
    for (const QString& fileName : requiredFiles) {
        QString filePath = modelPath + "/" + fileName;
        QFileInfo fileInfo(filePath);

        if (!fileInfo.exists()) {
            qDebug() << "" << fileName << "- File not found";
            return false;
        }

        if (fileInfo.size() == 0) {
            qDebug() << "" << fileName << "- File is empty";
            return false;
        }

        if (!fileInfo.isReadable()) {
            qDebug() << "" << fileName << "- File is not readable";
            return false;
        }

        qDebug() << "✓" << fileName << "- Size:" << fileInfo.size() << "bytes";
    }

    qDebug() << "========== Model Files Validation Completed ==========";
    return true;
}

void FaceRecognitionManager::cleanup()
{
    qDebug() << "FaceRecognitionManager: Cleanup started";

    if (m_faceDetHandle) {
        rockx_destroy(m_faceDetHandle);
        m_faceDetHandle = nullptr;
        qDebug() << "FaceRecognitionManager: Face detection handle destroyed";
    }

    if (m_faceLandmarkHandle) {
        rockx_destroy(m_faceLandmarkHandle);
        m_faceLandmarkHandle = nullptr;
        qDebug() << "FaceRecognitionManager: Face landmark handle destroyed";
    }

    if (m_faceRecognizeHandle) {
        rockx_destroy(m_faceRecognizeHandle);
        m_faceRecognizeHandle = nullptr;
        qDebug() << "FaceRecognitionManager: Face recognition handle destroyed";
    }

    m_initialized = false;
    qDebug() << "FaceRecognitionManager: Cleanup completed";
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

int FaceRecognitionManager::getTotalRegisteredFaces()
{
    return m_database->getTotalFaceCount();
}

void FaceRecognitionManager::logPerformance(const QString& operation, float timeMs)
{
    if (timeMs > 50) {  // 只记录耗时较长的操作
        qDebug() << QString("FaceRecognitionManager: %1 took %.2f ms").arg(operation).arg(timeMs);
    }
}

bool FaceRecognitionManager::initializeRockX()
{
    rockx_ret_t ret;

    qDebug() << "========== RockX Initialization Debug ==========";

    // 1. Set environment variable
    qDebug() << "Setting ROCKX_MODEL_PATH environment variable:" << m_modelPath;
    qputenv("ROCKX_MODEL_PATH", m_modelPath.toLocal8Bit());

    // Verify environment variable
    QByteArray envValue = qgetenv("ROCKX_MODEL_PATH");
    qDebug() << "Current ROCKX_MODEL_PATH value:" << QString::fromLocal8Bit(envValue);

    // 2. Try alternative path settings
    qDebug() << "Attempting to set alternative environment variables...";
    qputenv("ROCKX_DATA_PATH", m_modelPath.toLocal8Bit());
    qputenv("ROCKX_MODEL_DIR", m_modelPath.toLocal8Bit());

    // 3. Check current working directory
    QString oldWorkDir = QDir::currentPath();
    qDebug() << "Original working directory:" << oldWorkDir;

    // Try changing to model directory
    QDir::setCurrent(m_modelPath);
    qDebug() << "Changed to model directory:" << QDir::currentPath();

    // 4. Create RockX handles
    qDebug() << "Creating RockX handles...";

    // Create face detection handle
    qDebug() << "Creating face detection handle (ROCKX_MODULE_FACE_DETECTION)...";
    qDebug() << "Module ID:" << ROCKX_MODULE_FACE_DETECTION;

    ret = rockx_create(&m_faceDetHandle, ROCKX_MODULE_FACE_DETECTION, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << " Failed to create face detection handle!";
        qDebug() << "   Error code:" << ret;
        qDebug() << "   Error details:";
        switch (ret) {
        case -1:
            qDebug() << "     ROCKX_RET_FAIL (-1): General error";
            break;
        case -2:
            qDebug() << "     Possible model file not found";
            break;
        case -3:
            qDebug() << "     Possible memory allocation failure";
            break;
        default:
            qDebug() << "     Unknown error code:" << ret;
        }

        // List files in current directory
        qDebug() << "Current directory file list:";
        QDir currentDir(".");
        QStringList files = currentDir.entryList(QDir::Files);
        for (const QString& file : files) {
            if (file.contains("face_detection")) {
                qDebug() << "   Found related file:" << file;
            }
        }

        // Restore working directory
        QDir::setCurrent(oldWorkDir);
        return false;
    }
    qDebug() << "✓ Face detection handle created successfully";

    // Create face landmark handle
    qDebug() << "Creating face landmark handle (ROCKX_MODULE_FACE_LANDMARK_5)...";
    ret = rockx_create(&m_faceLandmarkHandle, ROCKX_MODULE_FACE_LANDMARK_5, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << " Failed to create face landmark handle, error code:" << ret;
        cleanup();
        QDir::setCurrent(oldWorkDir);
        return false;
    }
    qDebug() << "✓ Face landmark handle created successfully";

    // Create face recognition handle
    qDebug() << "Creating face recognition handle (ROCKX_MODULE_FACE_RECOGNIZE)...";
    ret = rockx_create(&m_faceRecognizeHandle, ROCKX_MODULE_FACE_RECOGNIZE, nullptr, 0);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << " Failed to create face recognition handle, error code:" << ret;
        cleanup();
        QDir::setCurrent(oldWorkDir);
        return false;
    }
    qDebug() << "✓ Face recognition handle created successfully";

    // Restore original working directory
    QDir::setCurrent(oldWorkDir);
    qDebug() << "Restored working directory:" << QDir::currentPath();

    qDebug() << "========== RockX Initialization Successful! ==========";
    return true;
}

// 图像格式转换（QImage -> rockx_image_t）
rockx_image_t FaceRecognitionManager::qImageToRockxImage(const QImage& image)
{
    rockx_image_t rockxImage;
    memset(&rockxImage, 0, sizeof(rockx_image_t));

    // 转换为RGB888格式
    QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);

    rockxImage.width = rgbImage.width();
    rockxImage.height = rgbImage.height();
    rockxImage.pixel_format = ROCKX_PIXEL_FORMAT_RGB888;
    rockxImage.data = (uint8_t*)rgbImage.bits();
    rockxImage.size = rgbImage.sizeInBytes();

    return rockxImage;
}

// 图像预处理
QImage FaceRecognitionManager::preprocessImage(const QImage& image)
{
    // 如果图像过大，进行缩放以提高处理速度
    if (image.width() > 1280 || image.height() > 720) {
        return image.scaled(1280, 720, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return image;
}

// 获取最大人脸（基于官方示例的get_max_face函数）
rockx_object_t* FaceRecognitionManager::getMaxFace(rockx_object_array_t* faceArray)
{
    if (faceArray->count == 0) {
        return nullptr;
    }

    rockx_object_t* maxFace = nullptr;
    int maxArea = 0;

    for (int i = 0; i < faceArray->count; i++) {
        rockx_object_t* curFace = &(faceArray->object[i]);

        int curArea = (curFace->box.right - curFace->box.left) *
                      (curFace->box.bottom - curFace->box.top);

        if (curArea > maxArea) {
            maxArea = curArea;
            maxFace = curFace;
        }
    }

    qDebug() << "FaceRecognitionManager: Selected max face with area:" << maxArea;
    return maxFace;
}

// 添加到 FaceRecognitionManager.cpp 中
QVector<FaceInfo> FaceRecognitionManager::detectFaces(const QImage& image)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || image.isNull()) {
        qDebug() << "FaceRecognitionManager: Not initialized or invalid image";
        return QVector<FaceInfo>();
    }

    QTime timer;
    timer.start();

    QVector<FaceInfo> faces = processDetection(image);

    m_lastDetectionTime = timer.elapsed();
    m_detectionCount++;

    logPerformance("Face Detection", m_lastDetectionTime);

    if (!faces.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Detected" << faces.size() << "faces";
        emit faceDetected(faces);
    }

    return faces;
}

QVector<FaceInfo> FaceRecognitionManager::processDetection(const QImage& image)
{
    QVector<FaceInfo> faces;

    // 1. 预处理图像
    QImage processedImage = preprocessImage(image);
    rockx_image_t rockxImage = qImageToRockxImage(processedImage);

    // 2. 执行人脸检测（基于官方示例）
    rockx_object_array_t faceArray;
    memset(&faceArray, 0, sizeof(rockx_object_array_t));

    rockx_ret_t ret = rockx_face_detect(m_faceDetHandle, &rockxImage, &faceArray, nullptr);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face detection failed, error:" << ret;
        return faces;
    }

    qDebug() << "FaceRecognitionManager: RockX detected" << faceArray.count << "faces";

    // 3. 解析检测结果
    for (int i = 0; i < faceArray.count; ++i) {
        const rockx_object_t& obj = faceArray.object[i];

        // 过滤低置信度的检测结果
        if (obj.score >= m_detectionThreshold) {
            FaceInfo faceInfo;
            faceInfo.bbox = QRect(obj.box.left, obj.box.top,
                                  obj.box.right - obj.box.left,
                                  obj.box.bottom - obj.box.top);
            faceInfo.confidence = obj.score;
            faceInfo.isRecognized = false;  // 仅检测，未识别

            faces.append(faceInfo);

            qDebug() << QString("FaceRecognitionManager: Face %1: bbox=(%2,%3,%4,%5) confidence=%.3f")
                            .arg(i)
                            .arg(faceInfo.bbox.x()).arg(faceInfo.bbox.y())
                            .arg(faceInfo.bbox.width()).arg(faceInfo.bbox.height())
                            .arg(faceInfo.confidence);
        } else {
            qDebug() << QString("FaceRecognitionManager: Face %1 filtered (confidence %.3f < threshold %.3f)")
            .arg(i).arg(obj.score).arg(m_detectionThreshold);
        }
    }

    return faces;
}

// 在 ai/FaceRecognitionManager.cpp 文件末尾添加这些方法

// ========== 人脸检测+识别组合功能 ==========
QVector<FaceInfo> FaceRecognitionManager::detectAndRecognizeFaces(const QImage& image)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || image.isNull()) {
        qDebug() << "FaceRecognitionManager: Not initialized or invalid image for recognition";
        return QVector<FaceInfo>();
    }

    QTime timer;
    timer.start();

    // 1. 首先进行人脸检测
    QVector<FaceInfo> detectedFaces = processDetection(image);

    if (detectedFaces.isEmpty()) {
        qDebug() << "FaceRecognitionManager: No faces detected for recognition";
        return detectedFaces;
    }

    // 2. 对每个检测到的人脸进行识别
    QVector<FaceInfo> recognizedFaces;
    for (int i = 0; i < detectedFaces.size(); ++i) {
        FaceInfo faceInfo = detectedFaces[i];

        // 对单个人脸进行识别
        FaceInfo recognizedFace = processRecognition(faceInfo, image);
        recognizedFaces.append(recognizedFace);

        // 发出相应信号
        if (recognizedFace.isRecognized) {
            emit faceRecognized(recognizedFace.personName, recognizedFace.similarity);
        } else {
            emit unknownFaceDetected(recognizedFace.bbox);
        }
    }

    m_lastRecognitionTime = timer.elapsed();
    m_recognitionCount++;

    logPerformance("Face Recognition", m_lastRecognitionTime);

    qDebug() << QString("FaceRecognitionManager: Recognition completed - %1 faces, %2 recognized")
                    .arg(recognizedFaces.size())
                    .arg(std::count_if(recognizedFaces.begin(), recognizedFaces.end(),
                                       [](const FaceInfo& f) { return f.isRecognized; }));

    return recognizedFaces;
}

// ========== 单个人脸识别处理 ==========
FaceInfo FaceRecognitionManager::processRecognition(const FaceInfo& detectedFace, const QImage& originalImage)
{
    FaceInfo faceInfo = detectedFace;

    qDebug() << "FaceRecognitionManager: Processing recognition for face at" << detectedFace.bbox;

    // 1. 从原图中裁剪人脸区域
    QRect faceRect = detectedFace.bbox;

    // 确保裁剪区域在图像边界内
    faceRect = faceRect.intersected(QRect(0, 0, originalImage.width(), originalImage.height()));

    if (faceRect.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Invalid face rectangle for recognition";
        return faceInfo;
    }

    QImage faceImage = originalImage.copy(faceRect);
    if (faceImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Failed to crop face image";
        return faceInfo;
    }

    // 2. 提取人脸特征
    QByteArray feature = extractFaceFeature(faceImage);
    if (feature.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Failed to extract face feature";
        return faceInfo;
    }

    qDebug() << "FaceRecognitionManager: Face feature extracted, size:" << feature.size();

    // 3. 在数据库中查找最佳匹配
    float similarity = 0.0f;
    int matchId = m_database->findBestMatch(feature, similarity, m_recognitionThreshold);

    if (matchId > 0 && similarity >= m_recognitionThreshold) {
        // 找到匹配的人脸
        FaceRecord record = m_database->getFaceRecord(matchId);
        if (record.id > 0) {
            faceInfo.personName = record.name;
            faceInfo.faceId = matchId;
            faceInfo.similarity = similarity;
            faceInfo.isRecognized = true;

            qDebug() << QString("FaceRecognitionManager: Face recognized as %1 (similarity: %.3f)")
                            .arg(record.name).arg(similarity);
        }
    } else {
        qDebug() << QString("FaceRecognitionManager: Unknown face (best similarity: %.3f, threshold: %.3f)")
        .arg(similarity).arg(m_recognitionThreshold);
    }

    return faceInfo;
}

// ========== 人脸注册功能 ==========
bool FaceRecognitionManager::registerFace(const QString& name, const QImage& faceImage)
{
    QMutexLocker locker(&m_mutex);

    if (!m_initialized || name.trimmed().isEmpty() || faceImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Invalid parameters for face registration";
        return false;
    }

    qDebug() << "FaceRecognitionManager: Starting face registration for:" << name;

    // 1. 检查是否已存在同名人脸
    QStringList existingNames = getAllRegisteredNames();
    if (existingNames.contains(name.trimmed(), Qt::CaseInsensitive)) {
        qDebug() << "FaceRecognitionManager: Face with name already exists:" << name;
        return false;
    }

    // 2. 提取人脸特征
    QByteArray feature = extractFaceFeature(faceImage);
    if (feature.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Failed to extract face feature for registration";
        return false;
    }

    qDebug() << "FaceRecognitionManager: Face feature extracted for registration, size:" << feature.size();

    // 3. 保存到数据库
    QString imagePath = QString("faces/%1_%2.jpg")
                            .arg(name.trimmed())
                            .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));

    QString description = QString("Registered face for %1").arg(name);

    if (!m_database->addFaceRecord(name.trimmed(), imagePath, feature, description)) {
        qDebug() << "FaceRecognitionManager: Failed to add face record to database:" << name;
        return false;
    }

    // 4. 保存图像文件 (可选)
    QString fullImagePath = QCoreApplication::applicationDirPath() + "/data/" + imagePath;
    QDir().mkpath(QFileInfo(fullImagePath).absolutePath());

    if (!faceImage.save(fullImagePath)) {
        qDebug() << "FaceRecognitionManager: Warning - failed to save face image:" << fullImagePath;
        // 即使图像保存失败，数据库记录已添加，仍然返回成功
    } else {
        qDebug() << "FaceRecognitionManager: Face image saved:" << fullImagePath;
    }

    qDebug() << "FaceRecognitionManager: Face registered successfully:" << name;
    return true;
}

QByteArray FaceRecognitionManager::extractFaceFeature(const QImage& faceImage)
{
    if (!m_initialized || faceImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Cannot extract feature - not initialized or invalid image";
        return QByteArray();
    }

    qDebug() << "FaceRecognitionManager: Starting feature extraction...";

    // 1. 转换图像格式
    QImage processedImage = preprocessImage(faceImage);
    rockx_image_t inputImage = qImageToRockxImage(processedImage);

    // 2. 人脸检测 (在特征提取前确保有人脸)
    rockx_object_array_t faceArray;
    memset(&faceArray, 0, sizeof(rockx_object_array_t));

    rockx_ret_t ret = rockx_face_detect(m_faceDetHandle, &inputImage, &faceArray, nullptr);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face detection failed during feature extraction, error:" << ret;
        return QByteArray();
    }

    if (faceArray.count == 0) {
        qDebug() << "FaceRecognitionManager: No face found during feature extraction";
        return QByteArray();
    }

    // 3. 选择最大的人脸 (基于官方示例的get_max_face)
    rockx_object_t* maxFace = getMaxFace(&faceArray);
    if (!maxFace) {
        qDebug() << "FaceRecognitionManager: Failed to get max face";
        return QByteArray();
    }

    qDebug() << QString("FaceRecognitionManager: Max face selected - bbox=(%1,%2,%3,%4) score=%.3f")
                    .arg(maxFace->box.left).arg(maxFace->box.top)
                    .arg(maxFace->box.right).arg(maxFace->box.bottom)
                    .arg(maxFace->score);

    // 4. 人脸对齐 (基于官方示例)
    rockx_image_t alignedImage;
    memset(&alignedImage, 0, sizeof(rockx_image_t));

    ret = rockx_face_align(m_faceLandmarkHandle, &inputImage, &(maxFace->box), nullptr, &alignedImage);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face align failed, error:" << ret;
        return QByteArray();
    }

    qDebug() << "FaceRecognitionManager: Face aligned successfully";

    // 5. 特征提取 (基于官方示例)
    rockx_face_feature_t feature;
    memset(&feature, 0, sizeof(rockx_face_feature_t));

    ret = rockx_face_recognize(m_faceRecognizeHandle, &alignedImage, &feature);

    // 6. 释放对齐图像资源
    rockx_image_release(&alignedImage);

    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face feature extraction failed, error:" << ret;
        return QByteArray();
    }

    // 7. 🔧 修复：将特征数据转换为QByteArray
    // RockX人脸特征通常是512个float，总共2048字节
    const int FEATURE_SIZE = 512 * sizeof(float);
    QByteArray featureData(reinterpret_cast<const char*>(feature.feature), FEATURE_SIZE);

    qDebug() << QString("FaceRecognitionManager: Feature extraction completed - size: %1 bytes")
                    .arg(featureData.size());

    return featureData;
}


void FaceRecognitionManager::testBasicFunctionality()
{
    qDebug() << "========== FaceRecognitionManager Basic Test ==========";

    if (!m_initialized) {
        qDebug() << "ERROR: Manager not initialized";
        return;
    }

    qDebug() << "✓ Initialization: OK";
    qDebug() << "✓ Database connection:" << (m_database->isConnected() ? "OK" : "FAILED");
    qDebug() << "✓ Registered faces:" << m_database->getTotalFaceCount();
    qDebug() << "✓ Detection threshold:" << m_detectionThreshold;
    qDebug() << "✓ Recognition threshold:" << m_recognitionThreshold;

    // 测试句柄状态
    qDebug() << "✓ Face detection handle:" << (m_faceDetHandle ? "OK" : "NULL");
    qDebug() << "✓ Face landmark handle:" << (m_faceLandmarkHandle ? "OK" : "NULL");
    qDebug() << "✓ Face recognition handle:" << (m_faceRecognizeHandle ? "OK" : "NULL");

    qDebug() << "========== Basic Test Completed ==========";
}
