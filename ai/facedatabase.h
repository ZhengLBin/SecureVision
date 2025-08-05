// ai/facedatabase.h
#ifndef FACEDATABASE_H
#define FACEDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QMutex>
#include <QVector>
#include <QSqlQuery>      // 新增：用于数据库查询
#include <QSqlError>      // 新增：用于错误处理
#include <QSqlDatabase>   // 新增：数据库操作
#include <QDateTime>
#include "aitypes.h"

class FaceDatabase : public QObject
{
    Q_OBJECT

public:
    explicit FaceDatabase(QObject *parent = nullptr);
    ~FaceDatabase();

    // 数据库初始化和管理
    bool initialize(const QString& dbPath = QString());
    bool isConnected() const;

    // 人脸记录管理
    bool addFaceRecord(const QString& name,
                       const QString& imagePath,
                       const QByteArray& feature,
                       const QString& description = QString());

    QVector<FaceRecord> getAllFaceRecords();
    FaceRecord getFaceRecord(int id);
    QByteArray getFaceFeature(int id);

    // 查询功能
    bool faceExists(const QString& name);
    int getTotalFaceCount();

    // 人脸识别核心功能
    int findBestMatch(const QByteArray& queryFeature,
                      float& bestSimilarity,
                      float minSimilarity = 0.7f);

    // 统计更新
    void updateLastSeen(int id);
    void incrementRecognitionCount(int id);

    // 特征相似度计算 - 设为public以便测试
    static float calculateFeatureSimilarity(const QByteArray& feature1, const QByteArray& feature2);
    static bool isValidFeature(const QByteArray& feature);

    // 测试访问 - 友元类声明
    friend class AIDetectionThread;

signals:
    void faceAdded(const QString& name, int id);
    void faceRecognized(const QString& name, int id, float similarity);
    void databaseError(const QString& error);

private:
    // 数据库管理
    bool createTables();
    bool createIndexes();
    QString generateConnectionName();

    // 日志功能
    void logError(const QString& operation, const QSqlError& error);
    void logDebug(const QString& message);

    // 成员变量
    QSqlDatabase m_database;        // 数据库连接
    QString m_databasePath;         // 数据库文件路径
    bool m_isConnected;             // 连接状态
    mutable QMutex m_mutex;         // 线程安全锁

    friend class AIDetectionThread;
};

#endif // FACEDATABASE_H
