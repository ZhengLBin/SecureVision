// ai/FaceDatabase.cpp
#include "facedatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QStandardPaths>
#include <QCoreApplication>

FaceDatabase::FaceDatabase(QObject *parent)
    : QObject(parent), m_isConnected(false)
{
}

FaceDatabase::~FaceDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool FaceDatabase::initialize(const QString& dbPath)
{
    QMutexLocker locker(&m_mutex);

    // 确保数据库目录存在
    QDir dbDir = QFileInfo(dbPath).absoluteDir();
    if (!dbDir.exists()) {
        if (!dbDir.mkpath(".")) {
            qDebug() << "Failed to create database directory:" << dbDir.absolutePath();
            return false;
        }
    }

    // 配置数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE", generateConnectionName());
    m_database.setDatabaseName(dbPath);

    if (!m_database.open()) {
        logError("Failed to open database", m_database.lastError());
        return false;
    }

    // 创建表结构
    if (!createTables()) {
        m_database.close();
        return false;
    }

    m_isConnected = true;
    qDebug() << "Face database initialized successfully:" << dbPath;
    return true;
}

bool FaceDatabase::createTables()
{
    QSqlQuery query(m_database);

    // 创建人脸记录表
    QString createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS face_records (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            image_path TEXT,
            feature BLOB NOT NULL,
            create_time DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_seen DATETIME,
            recognition_count INTEGER DEFAULT 0
        )
    )";

    if (!query.exec(createTableSQL)) {
        logError("Failed to create face_records table", query.lastError());
        return false;
    }

    // 创建索引以提高查询性能
    QString createIndexSQL = "CREATE INDEX IF NOT EXISTS idx_face_name ON face_records(name)";
    if (!query.exec(createIndexSQL)) {
        logError("Failed to create index", query.lastError());
        return false;
    }

    qDebug() << "Database tables created successfully";
    return true;
}

bool FaceDatabase::addFaceRecord(const QString& name, const QString& imagePath, const QByteArray& feature)
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected || name.isEmpty() || feature.isEmpty()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(R"(
        INSERT INTO face_records (name, image_path, feature, create_time, last_seen)
        VALUES (?, ?, ?, ?, ?)
    )");

    QDateTime now = QDateTime::currentDateTime();
    query.addBindValue(name);
    query.addBindValue(imagePath);
    query.addBindValue(feature);
    query.addBindValue(now);
    query.addBindValue(now);

    if (!query.exec()) {
        logError("Failed to add face record", query.lastError());
        return false;
    }

    emit faceAdded(name);
    qDebug() << "Face record added successfully:" << name;
    return true;
}

QVector<FaceRecord> FaceDatabase::getAllFaceRecords()
{
    QMutexLocker locker(&m_mutex);
    QVector<FaceRecord> records;

    if (!m_isConnected) {
        return records;
    }

    QSqlQuery query("SELECT id, name, image_path, create_time, last_seen, recognition_count FROM face_records", m_database);

    while (query.next()) {
        FaceRecord record;
        record.id = query.value(0).toInt();
        record.name = query.value(1).toString();
        record.imagePath = query.value(2).toString();
        record.createTime = query.value(3).toDateTime();
        record.lastSeen = query.value(4).toDateTime();
        record.recognitionCount = query.value(5).toInt();

        records.append(record);
    }

    return records;
}

int FaceDatabase::findBestMatch(const QByteArray& feature, float& similarity)
{
    QMutexLocker locker(&m_mutex);

    if (!m_isConnected || feature.isEmpty()) {
        similarity = 0.0f;
        return -1;
    }

    // 这里简化处理，实际应该使用RockX的人脸特征比对函数
    // 暂时返回模拟结果
    similarity = 0.0f;
    return -1;
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

QString FaceDatabase::generateConnectionName()
{
    static int connectionCounter = 0;
    return QString("FaceDB_%1_%2")
        .arg(QCoreApplication::applicationPid())
        .arg(++connectionCounter);
}

void FaceDatabase::logError(const QString& operation, const QSqlError& error)
{
    QString errorMsg = QString("%1: %2").arg(operation, error.text());
    qDebug() << "FaceDatabase Error:" << errorMsg;
    emit databaseError(errorMsg);
}
