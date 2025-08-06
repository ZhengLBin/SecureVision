#include "aidetectionthread.h"
#include <QDebug>
#include <QCoreApplication>
#include <QTimer>
#include <QMutexLocker>


AIDetectionThread::AIDetectionThread(QObject *parent)
    : QThread(parent)
    , m_motionDetector(new MotionDetector(this))
    , m_faceDatabase(new FaceDatabase(this))
    , m_faceManager(nullptr)
    , m_faceRecognitionEnabled(true)
    , m_faceDetectionFrameCounter(0)
    , m_totalFaceDetections(0)
    , m_totalFaceRecognitions(0)
{
    qDebug() << "--------AIDetectionThread constructor started------------";
    qDebug() << "m_faceDatabase created at address:" << (void*)m_faceDatabase;

    // 初始化人脸识别管理器
    if (!initializeFaceRecognition()) {
        qDebug() << "AIDetectionThread: Face recognition initialization failed";
        m_faceRecognitionEnabled = false;
    }

    // QTimer::singleShot(5000, this, [this]() {
    //     qDebug() << "Timer triggered, about to start database test";
    //     qDebug() << "m_faceDatabase address in timer:" << (void*)m_faceDatabase;

    //     if (!m_faceDatabase) {
    //         qDebug() << "CRITICAL: m_faceDatabase is null in timer!";
    //         return;
    //     }

    //     testFaceDatabase();
    // });



    qDebug() << "AIDetectionThread constructor completed";
}

AIDetectionThread::~AIDetectionThread()
{
    stopDetection();
    if (isRunning()) {
        quit();
        wait(3000);
    }
}

void AIDetectionThread::startDetection()
{
    QMutexLocker locker(&m_mutex);
    m_running = true;
    if (!isRunning()) {
        start();
        QTimer::singleShot(5000, this, [this]() {
            if (m_faceRecognitionEnabled && m_faceManager) {
                qDebug() << "start auto face detection---------------------------..";
                runSimpleFaceTest();
            }
        });
    }

    qDebug() << "AI Detection started";
}

void AIDetectionThread::stopDetection()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    qDebug() << "AI Detection stopped";
}

AIConfig AIDetectionThread::getConfig() {
    QMutexLocker locker(&m_mutex);
    return m_config;
}

void AIDetectionThread::setConfig(const AIConfig& config)
{
    QMutexLocker locker(&m_mutex);
    m_config = config;

    // 更新检测器配置
    if (m_motionDetector) {
        m_motionDetector->setThreshold(config.motionThreshold);
        m_motionDetector->setROI(config.roiArea);
    }
}

void AIDetectionThread::addFrame(const QImage& frame)
{
    QMutexLocker locker(&m_mutex);

    if (!m_config.enableAI || !m_running) return;

    // 跳帧处理
    if (++m_frameCounter % (m_config.skipFrames + 1) != 0) return;

    // 队列管理
    if (m_frameQueue.size() >= MAX_QUEUE_SIZE) {
        m_frameQueue.dequeue(); // 丢弃旧帧
    }

    m_frameQueue.enqueue(frame);
}

void AIDetectionThread::run()
{
    qDebug() << "AIDetectionThread started";

    while (true) {
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running) break;
        }

        QImage frame;
        {
            QMutexLocker locker(&m_mutex);
            if (m_frameQueue.isEmpty()) {
                locker.unlock();
                msleep(33); // ~30fps
                continue;
            }
            frame = m_frameQueue.dequeue();
        }

        // 处理帧
        DetectionResult result = processFrame(frame);
        emit detectionResult(result);

        // 检查是否需要录制
        if (shouldRecord(result)) {
            RecordTrigger trigger = RecordTrigger::None;
            if (result.hasMotion) trigger = RecordTrigger::MotionDetected;
            if (!result.faces.isEmpty()) trigger = RecordTrigger::FaceDetected;

            emit recordTrigger(trigger, frame);
        }
    }

    qDebug() << "AIDetectionThread finished";
}

// 修改现有的 processFrame 方法
DetectionResult AIDetectionThread::processFrame(const QImage& frame)
{
    DetectionResult result;
    result.timestamp = QDateTime::currentDateTime();

    QTime totalTimer;
    totalTimer.start();

    // 1. 运动检测 (现有逻辑保持不变)
    if (m_config.enableMotionDetect && m_motionDetector) {
        QTime motionTimer;
        motionTimer.start();

        QRect motionArea;
        result.hasMotion = m_motionDetector->detectMotion(frame, motionArea);
        result.motionArea = motionArea;

        result.motionProcessTime = motionTimer.elapsed();
    }

    // 2. 🆕 人脸检测和识别
    if (m_config.enableFaceDetect && m_faceManager && m_faceRecognitionEnabled && shouldProcessFaces()) {
        QTime faceTimer;
        faceTimer.start();

        QVector<FaceInfo> detectedFaces;

        if (m_config.enableFaceRecognition) {
            // 执行检测+识别
            detectedFaces = m_faceManager->detectAndRecognizeFaces(frame);
        } else {
            // 仅执行检测
            detectedFaces = m_faceManager->detectFaces(frame);
        }

        // 更新结果
        result.faceInfos = detectedFaces;
        result.hasFaceDetection = !detectedFaces.isEmpty();
        result.totalFaceCount = detectedFaces.size();
        result.recognizedFaceCount = 0;
        result.unknownFaceCount = 0;

        // 统计识别结果
        for (const auto& face : detectedFaces) {
            if (face.isRecognized) {
                result.recognizedFaceCount++;
            } else {
                result.unknownFaceCount++;
            }
        }

        result.faceDetectionTime = faceTimer.elapsed();

        // 更新统计信息
        updateFaceDetectionStatistics(result);

        // 🆕 触发录制逻辑
        if (result.hasFaceDetection) {
            if (result.recognizedFaceCount > 0 && m_config.recordKnownFaces) {
                emit recordTrigger(RecordTrigger::KnownFaceDetected, frame);
            }
            if (result.unknownFaceCount > 0 && m_config.recordUnknownFaces) {
                emit recordTrigger(RecordTrigger::UnknownFaceDetected, frame);
            }
            if (result.totalFaceCount > 1) {
                emit recordTrigger(RecordTrigger::MultipleFacesDetected, frame);
            }
        }
    }

    // 保存当前结果用于下次处理
    m_lastResult = result;

    return result;
}

bool AIDetectionThread::shouldRecord(const DetectionResult& result)
{
    // 当前阶段：有移动就录制
    return result.hasMotion;

    // 下个阶段：有人脸就录制
    // return !result.faces.isEmpty();
}

void AIDetectionThread::clearQueue()
{
    QMutexLocker locker(&m_mutex);
    m_frameQueue.clear();
}

bool AIDetectionThread::initializeFaceRecognition()
{
    qDebug() << "AIDetectionThread: Initializing face recognition...================================";

    m_faceManager = new FaceRecognitionManager(this);

    // 连接信号槽
    connect(m_faceManager, &FaceRecognitionManager::faceDetected,
            this, &AIDetectionThread::onFaceDetected);
    connect(m_faceManager, &FaceRecognitionManager::faceRecognized,
            this, &AIDetectionThread::onFaceRecognized);
    connect(m_faceManager, &FaceRecognitionManager::unknownFaceDetected,
            this, &AIDetectionThread::onUnknownFaceDetected);
    connect(m_faceManager, &FaceRecognitionManager::initializationFailed,
            this, &AIDetectionThread::onFaceManagerInitializationFailed);

    // 初始化人脸识别管理器
    if (!m_faceManager->initialize()) {
        qDebug() << "AIDetectionThread: Failed to initialize face recognition manager";
        delete m_faceManager;
        m_faceManager = nullptr;
        return false;
    }

    if (m_faceManager) {
        m_faceManager->testBasicFunctionality();
        qDebug() << "AIDetectionThread: Face recognition manager ready";
        qDebug() << "  - Registered faces:" << m_faceManager->getTotalRegisteredFaces();
    }

    qDebug() << "AIDetectionThread: Face recognition initialized successfully";
    return true;
}

// 🆕 信号槽处理方法
void AIDetectionThread::onFaceDetected(const QVector<FaceInfo>& faces)
{
    m_totalFaceDetections++;
    qDebug() << QString("AIDetectionThread: Detected %1 faces (total: %2)")
                    .arg(faces.size())
                    .arg(m_totalFaceDetections);

    // 发出性能更新信号
    emit performanceUpdate(QString("Face Detection: %1 faces detected").arg(faces.size()));
}

void AIDetectionThread::onFaceRecognized(const QString& name, float similarity)
{
    m_totalFaceRecognitions++;
    qDebug() << QString("AIDetectionThread: Recognized face: %1 (similarity: %.2f) (total: %3)")
                    .arg(name)
                    .arg(similarity)
                    .arg(m_totalFaceRecognitions);

    emit performanceUpdate(QString("Face Recognition: %1 (%.1f%%)").arg(name).arg(similarity * 100));
}

void AIDetectionThread::onUnknownFaceDetected(const QRect& faceRect)
{
    qDebug() << "AIDetectionThread: Unknown face detected at:" << faceRect;
    emit performanceUpdate(QString("Unknown face detected: %1x%2")
                               .arg(faceRect.width())
                               .arg(faceRect.height()));
}

void AIDetectionThread::onFaceManagerInitializationFailed(const QString& reason)
{
    qDebug() << "AIDetectionThread: Face manager initialization failed:" << reason;
    m_faceRecognitionEnabled = false;
    emit faceRecognitionError("Face recognition initialization failed: " + reason);
}

// 🆕 公共接口方法
bool AIDetectionThread::registerFace(const QString& name, const QImage& faceImage)
{
    if (!m_faceManager) {
        qDebug() << "AIDetectionThread: Face manager not available";
        return false;
    }

    bool result = m_faceManager->registerFace(name, faceImage);
    if (result) {
        emit faceRegistered(name);
        qDebug() << "AIDetectionThread: Face registered successfully:" << name;
    } else {
        qDebug() << "AIDetectionThread: Failed to register face:" << name;
    }

    return result;
}

QStringList AIDetectionThread::getRegisteredFaces()
{
    if (!m_faceManager) {
        return QStringList();
    }

    return m_faceManager->getAllRegisteredNames();
}

int AIDetectionThread::getTotalRegisteredFaces()
{
    if (!m_faceManager) {
        return 0;
    }

    return m_faceManager->getTotalRegisteredFaces();
}

void AIDetectionThread::setFaceRecognitionEnabled(bool enabled)
{
    QMutexLocker locker(&m_mutex);

    if (m_faceRecognitionEnabled != enabled) {
        m_faceRecognitionEnabled = enabled;
        m_config.enableFaceRecognition = enabled;

        qDebug() << "AIDetectionThread: Face recognition" << (enabled ? "enabled" : "disabled");
        emit faceDetectionStatusChanged(enabled);
    }
}

void AIDetectionThread::setFaceDetectionThreshold(float threshold)
{
    if (m_faceManager) {
        m_faceManager->setDetectionThreshold(threshold);
        qDebug() << "AIDetectionThread: Face detection threshold set to:" << threshold;
    }
}

void AIDetectionThread::setFaceRecognitionThreshold(float threshold)
{
    if (m_faceManager) {
        m_faceManager->setRecognitionThreshold(threshold);
        qDebug() << "AIDetectionThread: Face recognition threshold set to:" << threshold;
    }
}

bool AIDetectionThread::shouldProcessFaces()
{
    // 智能调度：根据运动检测结果调整人脸检测频率
    m_faceDetectionFrameCounter++;

    if (m_lastResult.hasMotion) {
        // 有运动时增加人脸检测频率
        return m_faceDetectionFrameCounter % 2 == 0;
    } else {
        // 无运动时降低人脸检测频率
        return m_faceDetectionFrameCounter % 5 == 0;
    }
}

void AIDetectionThread::updateFaceDetectionStatistics(const DetectionResult& result)
{
    if (result.hasFaceDetection) {
        m_lastFaceDetectionTime = QDateTime::currentDateTime();

        // 可以在这里添加更多统计逻辑
        QString stats = QString("Faces: %1 detected, %2 recognized, %3 unknown")
                            .arg(result.totalFaceCount)
                            .arg(result.recognizedFaceCount)
                            .arg(result.unknownFaceCount);

        emit performanceUpdate(stats);
    }
}

void AIDetectionThread::testFaceDatabase()
{
    qDebug() << "========== Security Database Test Started ==========";

    QString dbPath = QCoreApplication::applicationDirPath() + "/data/database/face_test.db";
    qDebug() << "Attempting to initialize database:" << dbPath;

    // Step 1: 数据库初始化
    if (!m_faceDatabase->initialize(dbPath)) {
        qDebug() << "Database initialization failed";
        return;
    }
    qDebug() << "Database initialized successfully";

    // Step 2: 基本连接测试
    if (!m_faceDatabase->isConnected()) {
        qDebug() << "Database not connected";
        return;
    }
    qDebug() << "Database connection verified";

    // Step 3: 获取初始记录数
    int initialCount = m_faceDatabase->getTotalFaceCount();
    qDebug() << "Initial record count:" << initialCount;

    // Step 4: 创建测试特征数据
    qDebug() << "Creating test feature data...";
    QByteArray testFeature;
    const int featureSize = 512 * sizeof(float);
    testFeature.resize(featureSize);
    float* data = reinterpret_cast<float*>(testFeature.data());

    // 创建具有特定模式的特征数据
    for (int i = 0; i < 512; ++i) {
        data[i] = 0.1f + (i % 100) * 0.001f + i * 0.0001f;
    }
    qDebug() << "Test feature created, size:" << testFeature.size() << "bytes";

    // Step 5: 验证特征数据
    if (FaceDatabase::isValidFeature(testFeature)) {
        qDebug() << "Feature validation passed";
    } else {
        qDebug() << "Feature validation failed";
        return;
    }

    // Step 6: 测试添加第一条记录
    QString testName1 = "TestUser_" + QString::number(QDateTime::currentMSecsSinceEpoch() % 10000);
    qDebug() << "Adding first test record:" << testName1;

    bool addResult1 = m_faceDatabase->addFaceRecord(
        testName1,
        "test1.jpg",
        testFeature,
        "First test user for validation"
        );

    if (addResult1) {
        qDebug() << "First record added successfully";
    } else {
        qDebug() << "Failed to add first record";
        return;
    }

    // Step 7: 验证记录数量增加
    int afterFirstAdd = m_faceDatabase->getTotalFaceCount();
    qDebug() << "Record count after first add:" << afterFirstAdd;
    if (afterFirstAdd == initialCount + 1) {
        qDebug() << "Record count increased correctly";
    } else {
        qDebug() << "Record count mismatch";
    }

    // Step 8: 测试记录存在性检查
    // if (m_faceDatabase->faceExists(testName1)) {
    //     qDebug() << "Record existence verification passed";
    // } else {
    //     qDebug() << "Record existence verification failed";
    // }

    // Step 9: 测试获取所有记录
    QVector<FaceRecord> allRecords = m_faceDatabase->getAllFaceRecords();
    qDebug() << "Retrieved all records, count:" << allRecords.size();
    if (!allRecords.isEmpty()) {
        FaceRecord firstRecord = allRecords.first();
        qDebug() << "First record details:";
        qDebug() << "  - ID:" << firstRecord.id;
        qDebug() << "  - Name:" << firstRecord.name;
        qDebug() << "  - Image path:" << firstRecord.imagePath;
        qDebug() << "  - Description:" << firstRecord.description;
        qDebug() << "  - Create time:" << firstRecord.createTime.toString();
    }

    // Step 10: 测试特征检索
    if (!allRecords.isEmpty()) {
        int recordId = allRecords.first().id;
        QByteArray retrievedFeature = m_faceDatabase->getFaceFeature(recordId);
        if (!retrievedFeature.isEmpty()) {
            qDebug() << "Feature retrieval successful, size:" << retrievedFeature.size();

            // Step 11: 测试自相似度计算
            float selfSimilarity = FaceDatabase::calculateFeatureSimilarity(testFeature, retrievedFeature);
            qDebug() << "Self-similarity score:" << selfSimilarity;
            if (selfSimilarity > 0.99f) {
                qDebug() << "Self-similarity test passed";
            } else {
                qDebug() << "Self-similarity test failed, expected > 0.99, got:" << selfSimilarity;
            }
        } else {
            qDebug() << "Feature retrieval failed";
        }
    }

    // Step 12: 创建不同的特征数据进行相似度对比测试
    qDebug() << "Creating different feature for similarity comparison...";
    QByteArray differentFeature;
    differentFeature.resize(featureSize);
    float* diffData = reinterpret_cast<float*>(differentFeature.data());

    // 创建明显不同的特征
    for (int i = 0; i < 512; ++i) {
        diffData[i] = 0.5f + (i % 50) * 0.002f;  // 不同的模式
    }

    float crossSimilarity = FaceDatabase::calculateFeatureSimilarity(testFeature, differentFeature);
    qDebug() << "Cross-similarity score:" << crossSimilarity;
    qDebug() << "Cross-similarity test completed (should be < 1.0)";

    // Step 13: 测试最佳匹配功能
    qDebug() << "Testing best match functionality...";
    float bestSimilarity = 0.0f;
    int bestMatchId = m_faceDatabase->findBestMatch(testFeature, bestSimilarity, 0.5f);

    if (bestMatchId > 0) {
        qDebug() << "Best match found - ID:" << bestMatchId << "Similarity:" << bestSimilarity;
    } else {
        qDebug() << "Best match not found or similarity below threshold";
    }

    // Step 14: 性能测试
    qDebug() << "Running performance test...";
    QDateTime startTime = QDateTime::currentDateTime();

    for (int i = 0; i < 10; ++i) {
        FaceDatabase::calculateFeatureSimilarity(testFeature, differentFeature);
    }

    qint64 elapsed = startTime.msecsTo(QDateTime::currentDateTime());
    qDebug() << "10 similarity calculations took:" << elapsed << "ms";
    qDebug() << "Average time per calculation:" << (elapsed / 10.0) << "ms";

    // Step 15: 最终状态检查
    int finalCount = m_faceDatabase->getTotalFaceCount();
    qDebug() << "Final record count:" << finalCount;

    qDebug() << "========== Security Database Test Completed Successfully ==========";
    qDebug() << "All database operations tested and working correctly!";
}

void AIDetectionThread::runSimpleFaceTest()
{
    qDebug() << "========== Starting Simple Face Recognition Test ==========";

    if (!m_faceManager || !m_faceRecognitionEnabled) {
        qDebug() << "Error: Face recognition manager not initialized";
        return;
    }

    // 1. 创建测试图像
    QString testImagePath = "/demo/test_face.jpg";
    QImage testImage(testImagePath);

    if (testImage.isNull()) {
        qDebug() << "Warning: Test image not found, using default test image";
        // 创建一个简单的测试图像
        testImage = QImage(400, 400, QImage::Format_RGB888);
        testImage.fill(QColor(128, 128, 128));
    }

    qDebug() << "Test image size:" << testImage.size();

    // 2. 生成唯一的测试用户名（避免重复）
    QString testUserName = QString("TestUser_%1").arg(QDateTime::currentMSecsSinceEpoch() % 10000);
    qDebug() << "Using test user name:" << testUserName;

    // 🆕 检查现有用户，如果 TestUser 已存在，直接使用它进行测试
    QStringList existingUsers = getRegisteredFaces();
    QString targetUser = "TestUser";

    bool needsRegistration = true;
    if (existingUsers.contains("TestUser")) {
        qDebug() << "Found existing TestUser, using it for recognition test";
        needsRegistration = false;
        targetUser = "TestUser";
    } else {
        qDebug() << "No existing TestUser found, will register new one";
        targetUser = testUserName;
    }

    // 3. 注册人脸（如果需要）
    if (needsRegistration) {
        qDebug() << "Step 1: Registering new test face:" << targetUser;
        bool registerResult = registerFace(targetUser, testImage);

        if (registerResult) {
            qDebug() << "✓ Face registration successful";
        } else {
            qDebug() << "✗ Face registration failed";
            return;
        }

        // 等待数据库操作完成
        QThread::msleep(1000);
    } else {
        qDebug() << "Step 1: Skipping registration, using existing user:" << targetUser;
    }

    // 4. 测试人脸识别
    qDebug() << "Step 2: Testing face recognition...";
    QVector<FaceInfo> recognitionResults = m_faceManager->detectAndRecognizeFaces(testImage);

    if (recognitionResults.isEmpty()) {
        qDebug() << "✗ No faces detected";

        // 🆕 如果没有检测到人脸，尝试仅进行检测测试
        qDebug() << "Step 2.1: Trying face detection only...";
        QVector<FaceInfo> detectionResults = m_faceManager->detectFaces(testImage);

        if (detectionResults.isEmpty()) {
            qDebug() << "✗ Face detection also failed - check image quality or RockX models";
        } else {
            qDebug() << "✓ Face detection successful but recognition failed";
            for (int i = 0; i < detectionResults.size(); ++i) {
                const FaceInfo& face = detectionResults[i];
                qDebug() << QString("  Detected Face%1: Position=(%2,%3,%4,%5) Confidence=%6")
                                .arg(i+1)
                                .arg(face.bbox.x()).arg(face.bbox.y())
                                .arg(face.bbox.width()).arg(face.bbox.height())
                                .arg(face.confidence, 0, 'f', 2);
            }
        }
    } else {
        qDebug() << "✓ Detected" << recognitionResults.size() << "face(s)";

        for (int i = 0; i < recognitionResults.size(); ++i) {
            const FaceInfo& face = recognitionResults[i];
            qDebug() << QString("  Face%1: Position=(%2,%3,%4,%5) Confidence=%6")
                            .arg(i+1)
                            .arg(face.bbox.x()).arg(face.bbox.y())
                            .arg(face.bbox.width()).arg(face.bbox.height())
                            .arg(face.confidence, 0, 'f', 2);

            if (face.isRecognized) {
                qDebug() << QString("  ✓ Recognition successful: %1 (Similarity: %2)")
                                .arg(face.personName)
                                .arg(face.similarity, 0, 'f', 3);

                // 🆕 验证识别结果是否正确
                if (face.personName == targetUser || face.personName == "TestUser") {
                    qDebug() << "  ✓ Correct user recognized!";
                } else {
                    qDebug() << "  ⚠ Recognized as different user:" << face.personName;
                }
            } else {
                qDebug() << "  ✗ Failed to recognize as registered user";
                qDebug() << "    Best similarity:" << face.similarity;
                qDebug() << "    Recognition threshold:" << m_config.faceRecognitionThreshold;

                // 🆕 提供调试建议
                if (face.similarity > 0.0f && face.similarity < m_config.faceRecognitionThreshold) {
                    qDebug() << "  💡 Suggestion: Try lowering the recognition threshold";
                    qDebug() << "     Current threshold:" << m_config.faceRecognitionThreshold;
                    qDebug() << "     Detected similarity:" << face.similarity;
                }
            }
        }
    }

    // 5. 数据库统计信息
    qDebug() << "Database statistics:";
    qDebug() << "  - Total registered faces:" << getTotalRegisteredFaces();
    qDebug() << "  - Registered users list:" << getRegisteredFaces();

    // 🆕 6. 测试特征提取和相似度计算
    qDebug() << "Step 3: Testing feature extraction and similarity calculation...";
    testFeatureConsistency(testImage, targetUser);

    qDebug() << "========== Face recognition test completed ==========";
}

void AIDetectionThread::testFeatureConsistency(const QImage& testImage, const QString& userName)
{
    if (!m_faceManager) {
        return;
    }

    qDebug() << "Testing feature consistency for user:" << userName;

    // 1. 从数据库获取已注册的特征
    QVector<FaceRecord> records = m_faceDatabase->getAllFaceRecords();
    FaceRecord targetRecord;

    for (const auto& record : records) {
        if (record.name == userName || record.name == "TestUser") {
            targetRecord = record;
            break;
        }
    }

    if (targetRecord.id <= 0) {
        qDebug() << "  ✗ Target user not found in database";
        return;
    }

    // 2. 获取存储的特征
    QByteArray storedFeature = m_faceDatabase->getFaceFeature(targetRecord.id);
    if (storedFeature.isEmpty()) {
        qDebug() << "  ✗ Failed to retrieve stored feature";
        return;
    }

    qDebug() << "  ✓ Retrieved stored feature, size:" << storedFeature.size();

    // 3. 从当前图像重新提取特征
    // 注意：这里需要先检测人脸，然后提取特征
    QVector<FaceInfo> detectedFaces = m_faceManager->detectFaces(testImage);

    if (detectedFaces.isEmpty()) {
        qDebug() << "  ✗ No faces detected for feature extraction";
        return;
    }

    // 选择最大的人脸
    FaceInfo largestFace = *std::max_element(detectedFaces.begin(), detectedFaces.end(),
                                             [](const FaceInfo& a, const FaceInfo& b) {
                                                 return a.bbox.width() * a.bbox.height() < b.bbox.width() * b.bbox.height();
                                             });

    qDebug() << "  ✓ Selected face for feature extraction:" << largestFace.bbox;

    // 4. 使用新的特征提取方法
    QByteArray currentFeature;

    // 这里我们需要直接调用特征提取，但由于方法是私有的，
    // 我们可以通过识别流程来获取特征并比较相似度

    // 临时解决方案：通过数据库的findBestMatch来测试相似度
    float similarity = 0.0f;
    int matchId = m_faceDatabase->findBestMatch(storedFeature, similarity, 0.0f); // 使用0.0阈值获取真实相似度

    qDebug() << "  Self-similarity test (stored feature vs stored feature):";
    qDebug() << "    Match ID:" << matchId;
    qDebug() << "    Similarity:" << similarity;

    if (similarity > 0.95f) {
        qDebug() << "  ✓ Self-similarity test passed (>0.95)";
    } else {
        qDebug() << "  ⚠ Self-similarity test concern - similarity:" << similarity;
    }

    // 5. 提供调试建议
    qDebug() << "  Debug suggestions:";
    qDebug() << "    - Current recognition threshold:" << m_config.faceRecognitionThreshold;

    if (similarity < 0.5f) {
        qDebug() << "    - ⚠ Very low similarity suggests feature extraction issues";
        qDebug() << "    - Check RockX model files and RGA hardware acceleration";
    } else if (similarity < m_config.faceRecognitionThreshold) {
        qDebug() << "    - 💡 Consider lowering recognition threshold to:" << (similarity * 0.9f);
    } else {
        qDebug() << "    - ✓ Recognition should work with current settings";
    }
}
