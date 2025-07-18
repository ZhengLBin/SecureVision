// ai/FaceDatabase.h
#ifndef FACEDATABASE_H
#define FACEDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVector>
#include <QMutex>
#include "aitypes.h"

class FaceDatabase : public QObject
{
    Q_OBJECT

public:
    explicit FaceDatabase(QObject *parent = nullptr);
    ~FaceDatabase();

    // 数据库管理
    bool initialize(const QString& dbPath = "data/database/face_recognition.db");
    bool isConnected() const { return m_isConnected; }

    // 人脸记录管理
    bool addFaceRecord(const QString& name, const QString& imagePath, const QByteArray& feature);
    bool deleteFaceRecord(int id);
    bool deleteFaceRecord(const QString& name);
    bool updateFaceRecord(int id, const QString& name, const QString& imagePath = QString());

    // 查询功能
    QVector<FaceRecord> getAllFaceRecords();
    FaceRecord getFaceRecord(int id);
    FaceRecord getFaceRecord(const QString& name);
    QByteArray getFaceFeature(int id);
    bool faceExists(const QString& name);

    // 统计功能
    int getTotalFaceCount();
    void updateLastSeen(int id);
    void incrementRecognitionCount(int id);

    // 特征匹配
    int findBestMatch(const QByteArray& feature, float& similarity);

signals:
    void databaseError(const QString& error);
    void faceAdded(const QString& name);
    void faceDeleted(const QString& name);

private:
    QSqlDatabase m_database;
    bool m_isConnected;
    QMutex m_mutex;

    bool createTables();
    QString generateConnectionName();
    void logError(const QString& operation, const QSqlError& error);
};

#endif // FACEDATABASE_H
