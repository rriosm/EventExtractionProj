
#include <QtTest>
#include <QSignalSpy>
#include "inc/UploadManager.h"

class UploadManagerTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Setup local temporary files for testing
        QFile file("test_video.mp4");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QByteArray(1024 * 1024, 'A')); // 1MB dummy file
            file.close();
        }
    }

    void testResumableUploadFlow() {
        // 1. Initialize
        UploadManager manager("mock_api_key");
        manager.setApiUrl(Url("http://localhost:8080/initiate").toString()); // Point to your mock

        // 2. Setup Signal Spies to track events
        QSignalSpy finishedSpy(&manager, &UploadManager::uploadFinished);
        QSignalSpy progressSpy(&manager, &UploadManager::progressChanged);
        QSignalSpy errorSpy(&manager, &UploadManager::errorOccurred);

        // 3. Add task and start
        manager.addUploadTask("test_video.mp4", "Unit Test Video");
        manager.startQueue();

        // 4. Wait for the upload to complete (Timeout after 10 seconds)
        // Note: If running in a thread, use a loop with QTest::qWait
        bool success = finishedSpy.wait(10000);

        // 5. Verifications
        if (errorSpy.count() > 0) {
            QFAIL(qPrintable(errorSpy.first().at(1).toString()));
        }

        QVERIFY2(success, "Upload timed out or failed to finish");
        QVERIFY(progressSpy.count() > 0); // Ensure progress was reported

        // Verify the resulting URI is what our Mock Server returned
        QString finalUri = finishedSpy.first().at(1).toString();
        QCOMPARE(finalUri, QString("https://done.com/file123"));
    }

    void cleanupTestCase() {
        QFile::remove("test_video.mp4");
    }
};

#include "UploadManagerTest.moc"
