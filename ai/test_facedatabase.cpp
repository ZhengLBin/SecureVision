// Temporary test file: test_facedatabase.cpp
// Created in the ai directory for testing database functionality

#include "facedatabase.h"
#include <QCoreApplication>
#include <QDebug>
#include <QByteArray>
#include <QDir>

/**
 * @brief Create mock face feature data
 *
 * Real face features are 512-dimensional float vectors extracted from RockX
 * Here we create some mock data for testing
 */
QByteArray createMockFaceFeature(const QString& personName) {
    QByteArray feature;
    feature.resize(512 * sizeof(float));  // 512 floats

    float* data = reinterpret_cast<float*>(feature.data());

    // Create different feature patterns for different people
    int seed = qHash(personName);
    srand(seed);

    for (int i = 0; i < 512; ++i) {
        data[i] = (rand() % 1000) / 1000.0f;  // Random number between 0-1
    }

    return feature;
}

/**
 * @brief Test basic database operations
 */
bool testDatabaseBasicOperations() {
    qDebug() << "========== Testing Basic Database Operations ==========";

    // 1. Create database instance
    FaceDatabase* db = new FaceDatabase();

    // 2. Initialize database
    QString testDbPath = QCoreApplication::applicationDirPath() + "/test_face.db";
    if (!db->initialize(testDbPath)) {
        qDebug() << "❌ Database initialization failed";
        delete db;
        return false;
    }
    qDebug() << "✅ Database initialized successfully";

    // 3. Test adding face records
    qDebug() << "\n--- Testing Add Face Record ---";

    // Add first face
    QByteArray feature1 = createMockFaceFeature("Zhang San");
    bool result1 = db->addFaceRecord("Zhang San", "/path/to/zhangsan.jpg", feature1, "Employee");
    qDebug() << "Add Zhang San:" << (result1 ? "✅ Success" : "❌ Failed");

    // Add second face
    QByteArray feature2 = createMockFaceFeature("Li Si");
    bool result2 = db->addFaceRecord("Li Si", "/path/to/lisi.jpg", feature2, "Visitor");
    qDebug() << "Add Li Si:" << (result2 ? "✅ Success" : "❌ Failed");

    // Test duplicate add (should fail)
    bool result3 = db->addFaceRecord("Zhang San", "/path/to/zhangsan2.jpg", feature1, "Duplicate");
    qDebug() << "Duplicate add Zhang San:" << (!result3 ? "✅ Correctly rejected" : "❌ Wrongly allowed");

    // 4. Test query functions
    qDebug() << "\n--- Testing Query Functions ---";

    int totalCount = db->getTotalFaceCount();
    qDebug() << "Total face count:" << totalCount;

    bool exists1 = db->faceExists("Zhang San");
    bool exists2 = db->faceExists("Wang Wu");
    qDebug() << "Zhang San exists:" << (exists1 ? "✅ Yes" : "❌ No");
    qDebug() << "Wang Wu exists:" << (!exists2 ? "✅ Correctly not exists" : "❌ Wrongly exists");

    // 5. Test getting all records
    qDebug() << "\n--- Testing Get All Records ---";
    QVector<FaceRecord> records = db->getAllFaceRecords();
    qDebug() << "Retrieved" << records.size() << "records:";

    for (const auto& record : records) {
        qDebug() << QString("  ID:%1, Name:%2, CreateTime:%3")
        .arg(record.id)
            .arg(record.name)
            .arg(record.createTime.toString("yyyy-MM-dd hh:mm:ss"));
    }

    delete db;

    // Remove test database file
    QFile::remove(testDbPath);

    return result1 && result2 && !result3 && exists1 && !exists2 && (totalCount == 2);
}

/**
 * @brief Test feature matching functionality
 */
bool testFeatureMatching() {
    qDebug() << "\n========== Testing Feature Matching ==========";

    FaceDatabase* db = new FaceDatabase();
    QString testDbPath = QCoreApplication::applicationDirPath() + "/test_matching.db";

    if (!db->initialize(testDbPath)) {
        qDebug() << "❌ Database initialization failed";
        delete db;
        return false;
    }

    // 1. Add some test faces
    qDebug() << "\n--- Adding Test Faces ---";

    QByteArray feature_alice = createMockFaceFeature("Alice");
    QByteArray feature_bob = createMockFaceFeature("Bob");
    QByteArray feature_charlie = createMockFaceFeature("Charlie");

    db->addFaceRecord("Alice", "/alice.jpg", feature_alice);
    db->addFaceRecord("Bob", "/bob.jpg", feature_bob);
    db->addFaceRecord("Charlie", "/charlie.jpg", feature_charlie);

    qDebug() << "Added 3 face records";

    // 2. Test exact matching
    qDebug() << "\n--- Testing Exact Matching ---";

    float similarity;
    int matchId = db->findBestMatch(feature_alice, similarity, 0.5f);

    if (matchId > 0) {
        FaceRecord record = db->getFaceRecord(matchId);
        qDebug() << QString("✅ Match success: %1 (ID:%2, Similarity:%.3f)")
                        .arg(record.name).arg(matchId).arg(similarity);
    } else {
        qDebug() << "❌ Match failed";
    }

    // 3. Test unknown face
    qDebug() << "\n--- Testing Unknown Face ---";

    QByteArray unknown_feature = createMockFaceFeature("Unknown_Person");
    int unknownMatchId = db->findBestMatch(unknown_feature, similarity, 0.9f);  // High threshold

    qDebug() << QString("Unknown face match result: ID=%1, Similarity=%.3f")
                    .arg(unknownMatchId).arg(similarity);
    qDebug() << (unknownMatchId == -1 ? "✅ Correctly identified as unknown" : "❌ Wrong match");

    // 4. Test statistics functionality
    qDebug() << "\n--- Testing Statistics Functions ---";

    qDebug() << "Total recognition count update test:";
    int originalCount = db->getFaceRecord(1).recognitionCount;
    db->incrementRecognitionCount(1);
    int newCount = db->getFaceRecord(1).recognitionCount;
    qDebug() << QString("Original count:%1, New count:%2").arg(originalCount).arg(newCount);

    delete db;
    QFile::remove(testDbPath);

    return (matchId > 0) && (unknownMatchId == -1) && (newCount == originalCount + 1);
}

/**
 * @brief Main test function
 */
int testFaceDatabase() {
    qDebug() << "Starting face database functionality tests...";

    bool test1 = testDatabaseBasicOperations();
    bool test2 = testFeatureMatching();

    qDebug() << "\n========== Test Results Summary ==========";
    qDebug() << "Basic operations test:" << (test1 ? "✅ Passed" : "❌ Failed");
    qDebug() << "Feature matching test:" << (test2 ? "✅ Passed" : "❌ Failed");
    qDebug() << "Overall result:" << (test1 && test2 ? "✅ All tests passed" : "❌ Some tests failed");

    return (test1 && test2) ? 0 : 1;
}
