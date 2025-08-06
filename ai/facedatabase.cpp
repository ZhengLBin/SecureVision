// ai/facedatabase.cpp
#include "facedatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QVariant>
#include <cmath>

FaceDatabase::FaceDatabase(QObject *parent)
    : QObject(parent), m_isConnected(false)
{
    qDebug() << "FaceDatabase constructor started";
    qDebug() << "FaceDatabase object created at address:" << (void*)this;
    qDebug() << "Parent object:" << (void*)parent;
    qDebug() << "m_isConnected initialized to:" << m_isConnected;
    qDebug() << "FaceDatabase constructor completed successfully";
}

// 同时也为 isConnected 方法添加调试信息
bool FaceDatabase::isConnected() const
{
    qDebug() << "isConnected() called, m_isConnected value:" << m_isConnected;
    return m_isConnected;
}

FaceDatabase::~FaceDatabase()
{
    // 析构时关闭数据库连接
    if (m_database.isOpen()) {
        m_database.close();
        qDebug() << "Database connection closed";
    }
}

bool FaceDatabase::initialize(const QString& dbPath)
{
    QMutexLocker locker(&m_mutex);  // 线程安全保护

    // 1. 确定数据库文件路径
    if (dbPath.isEmpty()) {
        // 如果没有指定路径，使用默认路径
        QString appDir = QCoreApplication::applicationDirPath();
        m_databasePath = appDir + "/data/database/face_recognition.db";
    } else {
        m_databasePath = dbPath;
    }

    // 2. 确保数据库目录存在
    QDir dbDir = QFileInfo(m_databasePath).absoluteDir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            logError("Failed to create database directory", QSqlError());
            return false;
        }
        qDebug() << "Created database directory:" << dbDir.absolutePath();
    }

    // 3. 配置SQLite数据库连接
    // SQLite驱动名称是"QSQLITE"
    m_database = QSqlDatabase::addDatabase("QSQLITE", generateConnectionName());
    m_database.setDatabaseName(m_databasePath);

    // 4. 打开数据库连接
    if (!m_database.open()) {
        logError("Failed to open database", m_database.lastError());
        return false;
    }

    qDebug() << "Database opened successfully:" << m_databasePath;

    // 5. 创建表结构
    if (!createTables()) {
        m_database.close();
        return false;
    }

    // 6. 创建索引
    if (!createIndexes()) {
        qDebug() << "Warning: Failed to create indexes, but continuing...";
    }

    m_isConnected = true;
    qDebug() << "Face database initialized successfully";

    return true;
}

bool FaceDatabase::createTables()
{
    QSqlQuery query(m_database);

    // 创建人脸记录表的SQL语句
    // 详细解释每个字段的作用：
    QString createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS face_records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,  -- 主键，自动递增
            name TEXT UNIQUE NOT NULL,              -- 人员姓名，唯一且不能为空
            image_path TEXT,                        -- 人脸图片文件路径
            feature BLOB NOT NULL,                  -- 人脸特征数据（二进制大对象）
            description TEXT,                       -- 描述信息
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP,  -- 创建时间，默认当前时间
            last_seen DATETIME,                     -- 最后识别时间
            recognition_count INTEGER DEFAULT 0,    -- 识别次数，默认0
            is_active BOOLEAN DEFAULT 1             -- 是否激活，默认激活
        )
    )";

    if (!query.exec(createTableSQL)) {
        logError("Failed to create face_records table", query.lastError());
        return false;
    }

    qDebug() << "face_records table created successfully";
    return true;
}

bool FaceDatabase::createIndexes()
{
    QSqlQuery query(m_database);

    // 创建姓名索引，提高按姓名查询的性能
    QString createNameIndexSQL = "CREATE INDEX IF NOT EXISTS idx_face_name ON face_records(name)";
    if (!query.exec(createNameIndexSQL)) {
        logError("Failed to create name index", query.lastError());
        return false;
    }

    // 创建激活状态索引
    QString createActiveIndexSQL = "CREATE INDEX IF NOT EXISTS idx_face_active ON face_records(is_active)";
    if (!query.exec(createActiveIndexSQL)) {
        logError("Failed to create active index", query.lastError());
        return false;
    }

    qDebug() << "Database indexes created successfully";
    return true;
}

bool FaceDatabase::addFaceRecord(const QString& name,
                                 const QString& imagePath,
                                 const QByteArray& feature,
                                 const QString& description)
{
    qDebug() << "addFaceRecord: Method entered";
    QMutexLocker locker(&m_mutex);

    // 基本验证
    if (!m_isConnected) {
        qDebug() << "addFaceRecord: Database not connected";
        return false;
    }

    if (name.trimmed().isEmpty()) {
        qDebug() << "addFaceRecord: Name cannot be empty";
        return false;
    }

    if (feature.isEmpty()) {
        qDebug() << "addFaceRecord: Feature data cannot be empty";
        return false;
    }

    qDebug() << "addFaceRecord: Starting transaction for complete record insertion";

    // 🔧 使用事务确保数据一致性
    if (!m_database.transaction()) {
        qDebug() << "addFaceRecord: Failed to start transaction";
        return false;
    }

    // 🔧 一次性插入完整记录，包括BLOB数据
    QSqlQuery query(m_database);

    // 准备完整的INSERT语句
    QString insertSQL = "INSERT INTO face_records (name, image_path, feature, description, create_time, last_seen, recognition_count, is_active) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)";

    if (!query.prepare(insertSQL)) {
        qDebug() << "addFaceRecord: Failed to prepare INSERT statement";
        qDebug() << "SQL Error:" << query.lastError().text();
        m_database.rollback();
        return false;
    }

    // 绑定所有参数
    query.addBindValue(name.trimmed());                                     // name
    query.addBindValue(imagePath);                                          // image_path
    query.addBindValue(feature);                                            // feature (BLOB)
    query.addBindValue(description);                                        // description
    query.addBindValue(QDateTime::currentDateTime());                       // create_time
    query.addBindValue(QDateTime::currentDateTime());                       // last_seen
    query.addBindValue(0);                                                  // recognition_count
    query.addBindValue(true);                                               // is_active

    qDebug() << "addFaceRecord: Executing complete INSERT with all data";
    qDebug() << "  - Name:" << name.trimmed();
    qDebug() << "  - Image path:" << imagePath;
    qDebug() << "  - Feature size:" << feature.size() << "bytes";
    qDebug() << "  - Description:" << description;

    if (!query.exec()) {
        qDebug() << "addFaceRecord: Complete INSERT failed";
        qDebug() << "SQL Error:" << query.lastError().text();
        qDebug() << "SQL Error Type:" << query.lastError().type();
        m_database.rollback();
        return false;
    }

    // 提交事务
    if (!m_database.commit()) {
        qDebug() << "addFaceRecord: Failed to commit transaction";
        qDebug() << "SQL Error:" << m_database.lastError().text();
        return false;
    }

    int newId = query.lastInsertId().toInt();
    qDebug() << "addFaceRecord: Complete record inserted successfully with ID:" << newId;

    emit faceAdded(name.trimmed(), newId);
    return true;
}

QVector<FaceRecord> FaceDatabase::getAllFaceRecords()
{
    QMutexLocker locker(&m_mutex);
    QVector<FaceRecord> records;

    if (!m_isConnected) {
        return records;
    }

    // SQL查询语句：获取所有激活的人脸记录
    QSqlQuery query("SELECT id, name, image_path, description, create_time, last_seen, recognition_count, is_active FROM face_records WHERE is_active = 1 ORDER BY name", m_database);

    while (query.next()) {
        FaceRecord record;
        record.id = query.value(0).toInt();
        record.name = query.value(1).toString();
        record.imagePath = query.value(2).toString();
        record.description = query.value(3).toString();
        record.createTime = query.value(4).toDateTime();
        record.lastSeen = query.value(5).toDateTime();
        record.recognitionCount = query.value(6).toInt();
        record.isActive = query.value(7).toBool();

        records.append(record);
    }

    logDebug(QString("Retrieved %1 face records").arg(records.size()));
    return records;
}

QByteArray FaceDatabase::getFaceFeature(int id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected) {
        return QByteArray();
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT feature FROM face_records WHERE id = ? AND is_active = 1");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        return query.value(0).toByteArray();
    }

    return QByteArray();
}

bool FaceDatabase::faceExists(const QString& name)
{
    qDebug() << "faceExists: Method entered, checking name:" << name;
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected) {
        qDebug() << "faceExists: Database not connected";
        return false;
    }

    qDebug() << "faceExists: Creating query";
    QSqlQuery query(m_database);

    // 尝试最简单的查询，不使用参数绑定
    qDebug() << "faceExists: Testing simple query without parameters";
    if (!query.exec("SELECT COUNT(*) FROM face_records")) {
        qDebug() << "faceExists: Simple count query failed";
        qDebug() << "SQL Error:" << query.lastError().text();
        qDebug() << "SQL Error Type:" << query.lastError().type();
        return false;
    }
    qDebug() << "faceExists: Simple count query succeeded";

    // 现在尝试带WHERE条件但不用参数绑定的查询
    QString directSQL = QString("SELECT COUNT(*) FROM face_records WHERE name = '%1'").arg(name.trimmed());
    qDebug() << "faceExists: Testing direct SQL:" << directSQL;

    QSqlQuery directQuery(m_database);
    if (!directQuery.exec(directSQL)) {
        qDebug() << "faceExists: Direct SQL query failed";
        qDebug() << "SQL Error:" << directQuery.lastError().text();
        qDebug() << "SQL Error Type:" << directQuery.lastError().type();
        return false;
    }
    qDebug() << "faceExists: Direct SQL query succeeded";

    if (directQuery.next()) {
        int count = directQuery.value(0).toInt();
        qDebug() << "faceExists: Found" << count << "matching records";
        return count > 0;
    }

    qDebug() << "faceExists: No results from query";
    return false;
}

int FaceDatabase::getTotalFaceCount()
{
    qDebug() << "getTotalFaceCount: Checking connection status...";

    QMutexLocker locker(&m_mutex);

    // 详细的状态检查
    qDebug() << "getTotalFaceCount: Checking connection status...";

    if (!m_isConnected) {
        qDebug() << "getTotalFaceCount: Not connected";
        return 0;
    }
    qDebug() << "getTotalFaceCount: Connection status OK";

    if (!m_database.isOpen()) {
        qDebug() << "getTotalFaceCount: Database is not open";
        return 0;
    }
    qDebug() << "getTotalFaceCount: Database is open";

    // 使用更安全的查询方式
    qDebug() << "getTotalFaceCount: Creating query...";
    QSqlQuery query(m_database);

    qDebug() << "getTotalFaceCount: Preparing query...";
    if (!query.prepare("SELECT COUNT(*) FROM face_records WHERE is_active = 1")) {
        qDebug() << "getTotalFaceCount: Failed to prepare query:" << query.lastError().text();
        return 0;
    }

    qDebug() << "getTotalFaceCount: Executing query...";
    if (!query.exec()) {
        qDebug() << "getTotalFaceCount: Failed to execute query:" << query.lastError().text();
        qDebug() << "getTotalFaceCount: Database error:" << m_database.lastError().text();
        return 0;
    }

    qDebug() << "getTotalFaceCount: Query executed successfully";

    if (query.next()) {
        int count = query.value(0).toInt();
        qDebug() << "getTotalFaceCount: Retrieved count:" << count;
        return count;
    } else {
        qDebug() << "getTotalFaceCount: No result from query";
        return 0;
    }
}

void FaceDatabase::updateLastSeen(int id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected) return;

    QSqlQuery query(m_database);
    query.prepare("UPDATE face_records SET last_seen = ? WHERE id = ?");
    query.addBindValue(QDateTime::currentDateTime());
    query.addBindValue(id);

    if (!query.exec()) {
        logError("Failed to update last_seen", query.lastError());
    }
}

void FaceDatabase::incrementRecognitionCount(int id)
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected) return;

    QSqlQuery query(m_database);
    query.prepare("UPDATE face_records SET recognition_count = recognition_count + 1 WHERE id = ?");
    query.addBindValue(id);

    if (!query.exec()) {
        logError("Failed to increment recognition count", query.lastError());
    }
}

// ========== 核心特征匹配功能 ==========
int FaceDatabase::findBestMatch(const QByteArray& queryFeature,
                                float& bestSimilarity,
                                float minSimilarity)
{
    QMutexLocker locker(&m_mutex);

    // 1. 参数验证
    if (!m_isConnected || !isValidFeature(queryFeature)) {
        bestSimilarity = 0.0f;
        return -1;
    }

    bestSimilarity = 0.0f;
    int bestMatchId = -1;
    int comparedCount = 0;

    // 2. 获取所有激活的人脸特征进行比对
    QSqlQuery query("SELECT id, name, feature FROM face_records WHERE is_active = 1", m_database);

    while (query.next()) {
        int recordId = query.value(0).toInt();
        QString recordName = query.value(1).toString();
        QByteArray recordFeature = query.value(2).toByteArray();

        // 3. 验证记录中的特征数据
        if (!isValidFeature(recordFeature)) {
            qDebug() << "Invalid feature data for record ID:" << recordId;
            continue;
        }

        // 4. 计算相似度
        float similarity = calculateFeatureSimilarity(queryFeature, recordFeature);
        comparedCount++;

        // 5. 更新最佳匹配
        if (similarity > bestSimilarity) {
            bestSimilarity = similarity;
            bestMatchId = recordId;
        }

        logDebug(QString("Compared with %1 (ID:%2): similarity=%3")
                     .arg(recordName).arg(recordId).arg(similarity, 0, 'f', 3));
    }

    logDebug(QString("Feature matching completed: compared %1 faces, best similarity=%2, threshold=%3")
                 .arg(comparedCount).arg(bestSimilarity, 0, 'f', 3).arg(minSimilarity, 0, 'f', 3));

    // 6. 检查是否达到最小相似度阈值
    if (bestSimilarity < minSimilarity) {
        logDebug("Best similarity below threshold, treating as unknown face");
        bestMatchId = -1;
    } else if (bestMatchId > 0) {
        // 7. 更新识别统计信息
        updateLastSeen(bestMatchId);
        incrementRecognitionCount(bestMatchId);

        // 获取匹配的人员姓名
        FaceRecord record = getFaceRecord(bestMatchId);
        if (record.id > 0) {
            emit faceRecognized(record.name, bestMatchId, bestSimilarity);
        }
    }

    return bestMatchId;
}

// ========== 特征相似度计算 ==========
float FaceDatabase::calculateFeatureSimilarity(const QByteArray& feature1, const QByteArray& feature2)
{
    // 1. 验证特征数据
    if (feature1.size() != feature2.size() || feature1.size() == 0) {
        return 0.0f;
    }

    // 2. 将字节数组转换为浮点数数组
    // RockX的人脸特征是512个float类型的数值
    const float* f1 = reinterpret_cast<const float*>(feature1.data());
    const float* f2 = reinterpret_cast<const float*>(feature2.data());
    int dimension = feature1.size() / sizeof(float);

    if (dimension != 512) {
        qDebug() << "Warning: Feature dimension is not 512:" << dimension;
    }

    // 3. 计算余弦相似度
    // 公式：similarity = (A·B) / (|A| × |B|)
    // 其中 A·B 是点积，|A|和|B|是向量的模长

    float dotProduct = 0.0f;    // 点积
    float norm1 = 0.0f;         // 第一个向量的模长平方
    float norm2 = 0.0f;         // 第二个向量的模长平方

    for (int i = 0; i < dimension; ++i) {
        dotProduct += f1[i] * f2[i];      // 累加点积
        norm1 += f1[i] * f1[i];           // 累加第一个向量的平方
        norm2 += f2[i] * f2[i];           // 累加第二个向量的平方
    }

    // 4. 计算模长
    norm1 = sqrt(norm1);
    norm2 = sqrt(norm2);

    // 5. 避免除零错误
    if (norm1 == 0.0f || norm2 == 0.0f) {
        return 0.0f;
    }

    // 6. 计算余弦相似度
    float similarity = dotProduct / (norm1 * norm2);

    // 7. 余弦相似度的范围是[-1, 1]，我们将其映射到[0, 1]
    // 对于人脸特征，通常都是正值，所以直接使用原值
    return std::max(0.0f, similarity);
}

bool FaceDatabase::isValidFeature(const QByteArray& feature)
{
    // RockX人脸特征应该是512个float，总共2048字节
    const int expectedSize = 512 * sizeof(float);

    if (feature.size() != expectedSize) {
        qDebug() << "Invalid feature size:" << feature.size() << "expected:" << expectedSize;
        return false;
    }

    return true;
}

FaceRecord FaceDatabase::getFaceRecord(int id)
{
    QMutexLocker locker(&m_mutex);
    FaceRecord record;

    if (!m_isConnected) {
        return record;
    }

    QSqlQuery query(m_database);
    query.prepare("SELECT id, name, image_path, description, create_time, last_seen, recognition_count, is_active FROM face_records WHERE id = ?");
    query.addBindValue(id);

    if (query.exec() && query.next()) {
        record.id = query.value(0).toInt();
        record.name = query.value(1).toString();
        record.imagePath = query.value(2).toString();
        record.description = query.value(3).toString();
        record.createTime = query.value(4).toDateTime();
        record.lastSeen = query.value(5).toDateTime();
        record.recognitionCount = query.value(6).toInt();
        record.isActive = query.value(7).toBool();
    }

    return record;
}

QString FaceDatabase::generateConnectionName()
{
    // 生成唯一的数据库连接名称
    // 包含进程ID和时间戳，确保唯一性
    static int connectionCounter = 0;
    return QString("FaceDB_%1_%2_%3")
        .arg(QCoreApplication::applicationPid())
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(++connectionCounter);
}

void FaceDatabase::logError(const QString& operation, const QSqlError& error)
{
    QString errorMsg = QString("%1: %2").arg(operation, error.text());
    qDebug() << "FaceDatabase Error:" << errorMsg;
    emit databaseError(errorMsg);
}

void FaceDatabase::logDebug(const QString& message)
{
    qDebug() << "FaceDatabase:" << message;
}
