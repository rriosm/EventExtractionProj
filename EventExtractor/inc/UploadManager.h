#ifndef UPLOADMANAGER_H
#define UPLOADMANAGER_H

// Test this UploadManager using OtherFiles/mock_server.py
// put the following in EventExtractionController or in main
// manager.setApiUrl("http://localhost:8080/upload/v1beta/files");

#include <QObject>
#include <QQueue>
#include <QFile>

#ifndef USE_LIBCURL
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#else
// LibCurl Forward Declarations
typedef void CURL;

#ifndef CURL_TYPEOF_CURL_OFF_T
#include <curl/system.h>
#endif
#endif

#include "Url.h"
using namespace utils;

#include "inc/Defs.h"

struct HeaderContext;

struct UploadTask {
    QString filePath;
    QString displayName;
    qint32 retryCount = 0;

    // Default constructor (required by Qt containers like QQueue)
    UploadTask() = default;

    // Custom constructor to match your enqueue arguments
    UploadTask(const QString &path, const QString &name) :
        filePath(path),
        displayName(name),
        retryCount(0)
    {
        // TODO
    }
};

class UploadManager : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief UploadManager
         * @param apiKey
         * @param parent
         */
        explicit UploadManager(QObject *parent = nullptr);

        /**
         * @brief UploadManager
         * @param apiKey
         * @param parent
         */
        UploadManager(const QString &apiKey, QObject *parent = nullptr);

        /**
         * @brief ~UploadManager
         */
        virtual ~UploadManager();

        Q_DISABLE_COPY(UploadManager)

        /**
         * @brief addUploadTask
         * @param filePath
         * @param displayName
         */
        void addUploadTask(const QString &filePath, const QString &displayName);

        /**
         * @brief addUploadTasks
         * @param uploadTasks
         */
        void addUploadTasks(const QMap<QString, QString> uploadTasks);

        /**
         * @brief modelCode
         * @return
         */
        AIModels_t modelCode() const;

        /**
         * @brief apiKey
         * @return
         */
        QString apiKey() const;

        /**
         * @brief apiUrl
         * @return
         */
        Url apiUrl() const;

        /**
         * @brief setApiUrl
         * @param newApiUrl
         */
        void setApiUrl(const Url &newApiUrl);

        /**
         * @brief setModelCode
         * @param newModelCode
         */
        void setup(AIModels_t newModelCode, const QString &apiKey, const Url &apiUri);

    signals:

        /**
         * @brief allUploadsCompleted
         */
        void allUploadsCompleted();

        /**
         * @brief errorOccurred
         * @param fileName
         * @param error
         */
        void errorOccurred(QString fileName, QString error);

        /**
         * @brief progressChanged
         * @param fileName
         * @param percentage
         */
        void progressChanged(QString fileName, qint32 percentage);

        /**
         * @brief uploadFinished
         * @param fileName
         * @param fileUri
         */
        void uploadFinished(QString fileName, QString fileUri);

    public slots:

        void clearAllRemoteFiles();

        /**
         * @brief startQueue
         */
        void startQueue();

    private slots:

        /**
         * @brief processNext
         */
        void processNext();

        /**
         * @brief onInitFinished
         */
        void onInitFinished();

        /**
         * @brief onUploadFinished
         */
        void onUploadFinished();

        /**
         * @brief onUploadProgress
         * @param bytesSent
         * @param bytesTotal
         */
        void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

#ifdef USE_LIBCURL

        /**
         * @brief updateProgress
         * @param percentage
         */
        void updateProgress(qint32 percentage);
#endif

    private:

        /**
         * @brief getHeaderContext
         * @return
         */
        HeaderContext getHeaderContext();

#ifdef USE_LIBCURL

        bool checkFileState(const QString &fileUri);

        /**
         * @brief WriteCallback: Helper to handle Curl's C-style callbacks back into Qt strings
         * @param contents
         * @param size
         * @param nmemb
         * @param userp
         * @return
         */
        static size_t writeCallback(void* contents,
                                    size_t size,
                                    size_t nmemb,
                                    QByteArray* userp);

        /**
         * @brief ProgressCallback: Helper for progress tracking
         * @param clientp
         * @param dltotal
         * @param dlnow
         * @param ultotal
         * @param ulnow
         * @return
         */
        static int progressCallback(void* clientp,
                                    curl_off_t dltotal,
                                    curl_off_t dlnow,
                                    curl_off_t ultotal,
                                    curl_off_t ulnow);
#endif

    private:

        AIModels_t m_modelCode;
        QString m_apiKey;
        Url m_apiUrl;

        QQueue<UploadTask> m_queue;

        // Current Task State
        UploadTask m_currentTask;
        QFile *m_currentFile;

#ifndef USE_LIBCURL
        QNetworkAccessManager *m_manager;
        QNetworkReply *m_currentReply;
#else
#endif
};

#endif
