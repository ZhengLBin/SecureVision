// ai/FaceRecognitionManager.cpp
#include "facerecognitionmanager.h"
#include <QTime>
#include <QCoreApplication>
#include <QDir>
#include <cmath>        // 提供 std::isfinite 函数
#include <algorithm>    // 提供 std::max_element 函数

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
        qDebug() << "Model directory does not exist:" << modelPath;
        return false;
    }
    qDebug() << "✓ Model directory exists";

    // 🔧 仅检查人脸识别必需的模型文件
    QStringList requiredFiles = {
        "face_detection_v3_fast.data",  // 人脸检测模型
        "face_landmark5.data",          // 5点关键点模型
        "face_recognition.data"         // 人脸识别模型
    };

    qDebug() << "Checking required face recognition model files:";
    for (const QString& fileName : requiredFiles) {
        QString filePath = modelPath + "/" + fileName;
        QFileInfo fileInfo(filePath);

        if (!fileInfo.exists()) {
            qDebug() << fileName << "- File not found";
            return false;
        }

        if (fileInfo.size() == 0) {
            qDebug() << fileName << "- File is empty";
            return false;
        }

        if (!fileInfo.isReadable()) {
            qDebug() << fileName << "- File is not readable";
            return false;
        }

        qDebug() << "✓" << fileName << "- Size:" << fileInfo.size() << "bytes";
    }

    qDebug() << "========== Face Recognition Model Files Validation Completed ==========";
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

rockx_image_t FaceRecognitionManager::qImageToRockxImage(const QImage& image)
{
    rockx_image_t rockxImage;
    memset(&rockxImage, 0, sizeof(rockx_image_t));

    // 🔧 方案1：使用成员变量缓存图像数据，避免临时对象销毁
    m_imageBuffer = image.convertToFormat(QImage::Format_RGB888);

    // 验证图像转换是否成功
    if (m_imageBuffer.isNull()) {
        qDebug() << "FaceRecognitionManager: Image conversion failed";
        return rockxImage;
    }

    rockxImage.width = m_imageBuffer.width();
    rockxImage.height = m_imageBuffer.height();
    rockxImage.pixel_format = ROCKX_PIXEL_FORMAT_RGB888;
    rockxImage.data = (uint8_t*)m_imageBuffer.bits();
    rockxImage.size = m_imageBuffer.sizeInBytes();

    // 🔧 添加安全检查
    if (!rockxImage.data || rockxImage.size == 0) {
        qDebug() << "FaceRecognitionManager: Invalid image data";
        memset(&rockxImage, 0, sizeof(rockx_image_t));
        return rockxImage;
    }

    qDebug() << QString("FaceRecognitionManager: Image converted - size: %1x%2, data: %3 bytes")
                    .arg(rockxImage.width)
                    .arg(rockxImage.height)
                    .arg(rockxImage.size);

    return rockxImage;
}

// 🔧 改进的图像预处理方法
QImage FaceRecognitionManager::preprocessImage(const QImage& image)
{
    if (image.isNull()) {
        qDebug() << "FaceRecognitionManager: Input image is null";
        return QImage();
    }

    QImage processedImage = image;

    // 🔧 确保图像尺寸适中，避免RGA处理过大图像时出错
    const int MAX_WIDTH = 1280;
    const int MAX_HEIGHT = 720;

    if (processedImage.width() > MAX_WIDTH || processedImage.height() > MAX_HEIGHT) {
        qDebug() << QString("FaceRecognitionManager: Resizing image from %1x%2 to fit %3x%4")
        .arg(processedImage.width())
            .arg(processedImage.height())
            .arg(MAX_WIDTH)
            .arg(MAX_HEIGHT);

        processedImage = processedImage.scaled(MAX_WIDTH, MAX_HEIGHT,
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);
    }

    // 🔧 确保图像格式兼容
    if (processedImage.format() != QImage::Format_RGB888) {
        qDebug() << "FaceRecognitionManager: Converting to RGB888 format";
        processedImage = processedImage.convertToFormat(QImage::Format_RGB888);
    }

    // 🔧 验证处理结果
    if (processedImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Image preprocessing failed";
        return QImage();
    }

    qDebug() << QString("FaceRecognitionManager: Image preprocessed - final size: %1x%2")
                    .arg(processedImage.width())
                    .arg(processedImage.height());

    return processedImage;
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

    // 🔧 修复格式化问题
    if (maxFace) {
        qDebug() << QString("FaceRecognitionManager: Max face selected - bbox=(%1,%2,%3,%4) score=%5")
        .arg(maxFace->box.left).arg(maxFace->box.top)
            .arg(maxFace->box.right).arg(maxFace->box.bottom)
            .arg(maxFace->score, 0, 'f', 3);
    }

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

            qDebug() << QString("FaceRecognitionManager: Face %1: bbox=(%2,%3,%4,%5) confidence=%6")
                            .arg(i)
                            .arg(faceInfo.bbox.x()).arg(faceInfo.bbox.y())
                            .arg(faceInfo.bbox.width()).arg(faceInfo.bbox.height())
                            .arg(faceInfo.confidence, 0, 'f', 3);
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

    try {
        qDebug() << "FaceRecognitionManager: Processing recognition for face at" << detectedFace.bbox;

        // 提取特征
        QByteArray feature = extractFaceFeatureFromDetectedFace(originalImage, detectedFace.bbox);

        if (feature.isEmpty()) {
            qDebug() << "FaceRecognitionManager: Failed to extract face feature";
            return faceInfo;
        }

        qDebug() << "FaceRecognitionManager: Face feature extracted, size:" << feature.size();

        // 在数据库中查找最佳匹配
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

                qDebug() << QString("FaceRecognitionManager: Face recognized as %1 (similarity: %2)")
                                .arg(record.name).arg(similarity, 0, 'f', 3);
            }
        } else {
            qDebug() << QString("FaceRecognitionManager: Unknown face (best similarity: %1, threshold: %2)")
            .arg(similarity, 0, 'f', 3).arg(m_recognitionThreshold, 0, 'f', 3);
        }

    } catch (const std::exception& e) {
        qDebug() << "FaceRecognitionManager: Exception in processRecognition:" << e.what();
    } catch (...) {
        qDebug() << "FaceRecognitionManager: Unknown exception in processRecognition";
    }

    return faceInfo;
}
QByteArray FaceRecognitionManager::extractFaceFeatureFromDetectedFace(const QImage& originalImage, const QRect& faceRect)
{
    if (!m_initialized || originalImage.isNull() || faceRect.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Invalid parameters for feature extraction";
        return QByteArray();
    }

    qDebug() << "FaceRecognitionManager: Extracting feature from detected face...";
    qDebug() << "Original image size:" << originalImage.size();
    qDebug() << "Face rect:" << faceRect;

    // 1. 预处理原始图像（确保与注册时使用相同的处理方式）
    QImage processedImage = preprocessImage(originalImage);
    if (processedImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Image preprocessing failed";
        return QByteArray();
    }

    // 2. 转换为RockX图像格式
    rockx_image_t inputImage = qImageToRockxImage(processedImage);
    if (!inputImage.data || inputImage.size == 0) {
        qDebug() << "FaceRecognitionManager: Failed to convert image format";
        return QByteArray();
    }

    // 3. 计算缩放比例（原始图像 -> 预处理图像）
    float scaleX = float(processedImage.width()) / originalImage.width();
    float scaleY = float(processedImage.height()) / originalImage.height();

    qDebug() << QString("FaceRecognitionManager: Scale factors - X: %1, Y: %2")
                    .arg(scaleX, 0, 'f', 3).arg(scaleY, 0, 'f', 3);

    // 4. 调整人脸框到预处理后的图像坐标系
    QRect scaledFaceRect(
        int(faceRect.x() * scaleX),
        int(faceRect.y() * scaleY),
        int(faceRect.width() * scaleX),
        int(faceRect.height() * scaleY)
        );

    // 确保调整后的人脸框在图像边界内
    scaledFaceRect = scaledFaceRect.intersected(QRect(0, 0, processedImage.width(), processedImage.height()));

    if (scaledFaceRect.isEmpty()) {
        qDebug() << "FaceRecognitionManager: Scaled face rect is empty";
        return QByteArray();
    }

    qDebug() << "FaceRecognitionManager: Scaled face rect:" << scaledFaceRect;

    // 5. 转换为RockX人脸框格式
    rockx_rect_t rockxBox;
    rockxBox.left = scaledFaceRect.x();
    rockxBox.top = scaledFaceRect.y();
    rockxBox.right = scaledFaceRect.right();
    rockxBox.bottom = scaledFaceRect.bottom();

    // 6. 进行人脸对齐（跳过重复检测步骤）
    rockx_image_t alignedImage;
    memset(&alignedImage, 0, sizeof(rockx_image_t));

    qDebug() << "FaceRecognitionManager: Starting face alignment with detected box...";
    rockx_ret_t ret = rockx_face_align(m_faceLandmarkHandle, &inputImage, &rockxBox, nullptr, &alignedImage);

    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face align failed, error:" << ret;

        // 🔧 RGA失败时的备选方案：直接使用裁剪的人脸区域
        qDebug() << "FaceRecognitionManager: Using cropped face as fallback...";

        QImage croppedFace = processedImage.copy(scaledFaceRect);
        if (croppedFace.isNull()) {
            qDebug() << "FaceRecognitionManager: Failed to crop face image";
            return QByteArray();
        }

        // 调整到标准人脸识别尺寸（通常为112x112）
        croppedFace = croppedFace.scaled(112, 112, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        alignedImage = qImageToRockxImage(croppedFace);

        if (!alignedImage.data) {
            qDebug() << "FaceRecognitionManager: Failed to convert cropped face";
            return QByteArray();
        }

        qDebug() << "FaceRecognitionManager: Using cropped face instead of aligned face";
    } else {
        qDebug() << "FaceRecognitionManager: Face aligned successfully";
    }

    // 7. 提取人脸特征
    rockx_face_feature_t feature;
    memset(&feature, 0, sizeof(rockx_face_feature_t));

    qDebug() << "FaceRecognitionManager: Extracting face features from aligned image...";
    ret = rockx_face_recognize(m_faceRecognizeHandle, &alignedImage, &feature);

    // 8. 安全释放对齐图像资源
    if (alignedImage.data && alignedImage.size > 0) {
        rockx_image_release(&alignedImage);
    }

    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face feature extraction failed, error:" << ret;
        return QByteArray();
    }

    // 9. 转换为QByteArray格式
    const int FEATURE_SIZE = 512 * sizeof(float);
    QByteArray featureData(reinterpret_cast<const char*>(feature.feature), FEATURE_SIZE);

    // 10. 验证特征质量
    if (!validateFeatureQuality(featureData)) {
        qDebug() << "FaceRecognitionManager: Feature quality validation failed";
        return QByteArray();
    }

    qDebug() << QString("FaceRecognitionManager: Feature extraction completed successfully - size: %1 bytes")
                    .arg(featureData.size());

    return featureData;
}

bool FaceRecognitionManager::validateFeatureQuality(const QByteArray& feature)
{
    if (feature.size() != 512 * sizeof(float)) {
        qDebug() << "FaceRecognitionManager: Invalid feature size:" << feature.size();
        return false;
    }

    const float* data = reinterpret_cast<const float*>(feature.data());

    // 检查特征向量的基本统计特性
    float sum = 0.0f;
    float sumSquares = 0.0f;
    int nonZeroCount = 0;

    for (int i = 0; i < 512; ++i) {
        float val = data[i];

        // 检查是否包含无效值
        if (!std::isfinite(val)) {
            qDebug() << "FaceRecognitionManager: Feature contains invalid value at index" << i;
            return false;
        }

        sum += val;
        sumSquares += val * val;

        if (val != 0.0f) {
            nonZeroCount++;
        }
    }

    // 计算均值和方差
    float mean = sum / 512.0f;
    float variance = (sumSquares / 512.0f) - (mean * mean);

    qDebug() << QString("FaceRecognitionManager: Feature statistics - Mean: %1, Variance: %2, NonZero: %3/512")
                    .arg(mean, 0, 'f', 6).arg(variance, 0, 'f', 6).arg(nonZeroCount);

    // 基本质量检查
    if (nonZeroCount < 100) {  // 至少20%的元素非零
        qDebug() << "FaceRecognitionManager: Too few non-zero elements in feature";
        return false;
    }

    if (variance < 1e-6) {  // 方差不能太小
        qDebug() << "FaceRecognitionManager: Feature variance too small";
        return false;
    }

    return true;
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
    qDebug() << "Input image size:" << faceImage.size() << "format:" << faceImage.format();

    // 🔧 1. 改进的图像预处理
    QImage processedImage = preprocessImage(faceImage);
    if (processedImage.isNull()) {
        qDebug() << "FaceRecognitionManager: Image preprocessing failed";
        return QByteArray();
    }

    // 🔧 2. 安全的图像格式转换
    rockx_image_t inputImage = qImageToRockxImage(processedImage);
    if (!inputImage.data || inputImage.size == 0) {
        qDebug() << "FaceRecognitionManager: Failed to convert image format";
        return QByteArray();
    }

    // 3. 人脸检测 (确保有人脸)
    rockx_object_array_t faceArray;
    memset(&faceArray, 0, sizeof(rockx_object_array_t));

    qDebug() << "FaceRecognitionManager: Starting face detection for feature extraction...";
    rockx_ret_t ret = rockx_face_detect(m_faceDetHandle, &inputImage, &faceArray, nullptr);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face detection failed during feature extraction, error:" << ret;
        return QByteArray();
    }

    if (faceArray.count == 0) {
        qDebug() << "FaceRecognitionManager: No face found during feature extraction";
        return QByteArray();
    }

    qDebug() << "FaceRecognitionManager: Found" << faceArray.count << "faces for feature extraction";

    // 4. 选择最大的人脸
    rockx_object_t* maxFace = getMaxFace(&faceArray);
    if (!maxFace) {
        qDebug() << "FaceRecognitionManager: Failed to get max face";
        return QByteArray();
    }

    qDebug() << QString("FaceRecognitionManager: Max face selected - bbox=(%1,%2,%3,%4) score=%.3f")
                    .arg(maxFace->box.left).arg(maxFace->box.top)
                    .arg(maxFace->box.right).arg(maxFace->box.bottom)
                    .arg(maxFace->score);

    // 🔧 5. 改进的人脸对齐，增加错误处理
    rockx_image_t alignedImage;
    memset(&alignedImage, 0, sizeof(rockx_image_t));

    qDebug() << "FaceRecognitionManager: Starting face alignment...";
    ret = rockx_face_align(m_faceLandmarkHandle, &inputImage, &(maxFace->box), nullptr, &alignedImage);
    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face align failed, error:" << ret;
        qDebug() << "This might be due to RGA hardware acceleration issues";

        // 🔧 如果RGA失败，可以尝试跳过对齐直接进行识别
        qDebug() << "FaceRecognitionManager: Attempting feature extraction without alignment...";

        // 裁剪人脸区域作为替代方案
        QRect faceRect(maxFace->box.left, maxFace->box.top,
                       maxFace->box.right - maxFace->box.left,
                       maxFace->box.bottom - maxFace->box.top);

        // 确保裁剪区域在图像边界内
        faceRect = faceRect.intersected(QRect(0, 0, processedImage.width(), processedImage.height()));

        if (faceRect.isEmpty()) {
            qDebug() << "FaceRecognitionManager: Invalid face rectangle";
            return QByteArray();
        }

        QImage croppedFace = processedImage.copy(faceRect);
        if (croppedFace.isNull()) {
            qDebug() << "FaceRecognitionManager: Failed to crop face image";
            return QByteArray();
        }

        // 重新转换裁剪后的图像
        alignedImage = qImageToRockxImage(croppedFace);
        if (!alignedImage.data) {
            qDebug() << "FaceRecognitionManager: Failed to convert cropped face image";
            return QByteArray();
        }

        qDebug() << "FaceRecognitionManager: Using cropped face instead of aligned face";
    } else {
        qDebug() << "FaceRecognitionManager: Face aligned successfully";
    }

    // 6. 特征提取
    rockx_face_feature_t feature;
    memset(&feature, 0, sizeof(rockx_face_feature_t));

    qDebug() << "FaceRecognitionManager: Extracting face features...";
    ret = rockx_face_recognize(m_faceRecognizeHandle, &alignedImage, &feature);

    // 🔧 7. 安全释放对齐图像资源
    if (alignedImage.data && alignedImage.size > 0) {
        // 只有当alignedImage是由RockX分配的才需要释放
        // 如果是我们自己转换的图像，则不需要调用rockx_image_release
        if (ret == ROCKX_RET_SUCCESS) {
            rockx_image_release(&alignedImage);
        }
    }

    if (ret != ROCKX_RET_SUCCESS) {
        qDebug() << "FaceRecognitionManager: Face feature extraction failed, error:" << ret;
        return QByteArray();
    }

    // 8. 将特征数据转换为QByteArray
    const int FEATURE_SIZE = 512 * sizeof(float);
    QByteArray featureData(reinterpret_cast<const char*>(feature.feature), FEATURE_SIZE);

    qDebug() << QString("FaceRecognitionManager: Feature extraction completed successfully - size: %1 bytes")
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
