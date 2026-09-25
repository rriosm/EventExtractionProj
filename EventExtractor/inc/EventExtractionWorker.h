#ifndef EVENTEXTRACTIONWORKER_H
#define EVENTEXTRACTIONWORKER_H

#include <QObject>
#include <QFile>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QMimeType>
#include <QPair>

#include <QProgressBar>

#include <QString>
#include <QStringList>
#include <QThread>

// Conditional Includes
#ifndef USE_LIBCURL
// QNAM Headers
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
#else
// LibCurl Forward Declarations
typedef void CURL;
#endif

#include "inc/Defs.h"
#include "inc/MessageHistory.h"
#include "inc/UploadManager.h"

// Assuming this struct is in your header or a shared types file
struct SoccerEvent {
    int timestamp;
    QString type;
    QString description;
};

class EventExtractionWorker : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Event Extraction Worker")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief EventExtraction
         * @param parent
         */
        explicit EventExtractionWorker(QObject *parent = nullptr);

        /**
         * @brief ~EventExtraction
         */
        virtual ~EventExtractionWorker();

        Q_DISABLE_COPY(EventExtractionWorker)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const EventExtractionWorker &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const EventExtractionWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const EventExtractionWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const EventExtractionWorker *ptr) const;

        // --- Configuration ---

        /**
         * @brief apiKey
         * @return
         */
        QString apiKey() const;

        /**
         * @brief setApiKey
         * @param key
         */
        void setApiKey(const QString &key);

        /**
         * @brief apiUrl
         * @return
         */
        Url &apiUrl();

        /**
         * @brief setApiUrl
         * @param apiUrl
         */
        void setApiUrl(const Url &apiUrl);

        /**
         * @brief modelStr
         * @return
         */
        QString modelStr() const;

        /**
         * @brief setModelStr
         * @param newModelStr
         */
        void setModelStr(const QString &newModelStr);

        /**
         * @brief contentType
         * @return
         */
        const QString &contentType() const;

        /**
         * @brief setContentType
         * @param newContentType
         */
        void setContentType(ContentMimeType_e newContentType);

        /**
         * @brief setModel
         * @param newModel
         * @param customModelString
         * @param customAPIUrl
         * @param customAPIKey
         */
        void setModel(AIModels_t newModel,
                      const QString &customModelString = QString(),
                      const QString &customAPIUrl = QString(),
                      const QString &customAPIKey = QString());

        // Optional: Set custom headers (Referer, Title) required by OpenRouter and other services

        /**
         * @brief setSiteInfo
         * @param newSiteUrl
         * @param newSiteName
         */
        void setSiteInfo(const Url &newSiteUrl, const QString &newSiteName = QString());

        // --- Data Preparation ---

        /**
         * @brief segmentId
         * @return
         */
        QString segmentId() const;

        /**
         * @brief setSegmentId
         * @param segmentId
         */
        void setSegmentId(const QString &segmentId);

        /**
         * @brief setSystemPrompt: Sets the "system" role message.
         * @param prompt
         */
        void setSystemPrompt(const QString &prompt);

        /**
         * @brief addUserMessage Adds a "user" role message.
         */
        void addUserMessage(const QString &content);

        /**
         * @brief addUserMessageWithAudio
         * @param text
         * @param audioPath
         */
        void addUserMessageWithAudio(const QString &text, const QString &audioPath);

        /**
         * @brief addUserMessageWithImage
         * @param text
         * @param imagePath
         */
        void addUserMessageWithImage(const QString &text, const QString &imagePath);

        /**
         * @brief addUserMessageWithVideo
         * @param text
         * @param videoPath
         */
        void addUserMessageWithVideo(const QString &text, const QString &videoPath);

        /**
         * @brief addUserMessageWithVideosList
         * @param text
         * @param videosPaths
         */
        void addUserMessageWithVideosList(const QString &text, const QStringList &videosPaths);

        /**
         * @brief addMessages
         * @param messages
         */
        void addMessageHistory(const MessageHistory &newMsgHistory);

        /**
         * @brief clearMessages Clears the conversation history.
         */
        void clearMessages();

        // --- Output Configuration ---
        /**
         * @brief setOutputFile If set, the response will be streamed to this file.
         * If empty, response is stored in memory.
         */
        void setOutputFile(const QString &filePath);

    signals:

        /**
         * @brief errorOccurred Emitted on libcurl errors.
         */
        void errorOccurred(const QString &errorMsg);

        /**
         * @brief eventsProcessed
         */
        void eventsProcessed(QList<SoccerEvent>);

        /**
         * @brief fileSaved Emitted if output file was used.
         */
        void fileSaved(const QString &filePath);

        /**
         * @brief finished Emitted when cURL completes successfully.
         * @param success True if HTTP code was 200-299.
         */
        void finished(bool success);

        /**
         * @brief percentageUpdated
         * @param percentage
         */
        void percentageUpdated(float progress);

        /**
         * @brief progressUpdated
         * @param bytesSent
         * @param bytesTotal
         */
        void progressUpdated(qint64 bytesSent, qint64 bytesTotal);

        /**
         * @brief responseReceived  Emitted with the full JSON response
         *        text (if not saving to file).
         * @param clipIndex
         * @param jsonResponse
         */
        void responseReceived(const QString &clipIndex, const QString &jsonResponse);

        /**
         * @brief started Emitted when cURL begins.
         */
        void started();

        /**
         * @brief stopRequested
         */
        void stopUploadRequested();

        /**
         * @brief statusUpdated
         * @param status
         */
        void statusUpdated(const QString &status);

        /**
         * @brief usageUpdated
         * @param dailyRequestCount
         * @param dailyLimit
         */
        void usageUpdated(qint32 currentRequestCount, qint32 dailyLimit);

    public slots:

        // --- Execution ---
        /**
         * @brief process Executes the cURL request synchronously.
         * NOTE: In a GUI application, wrap this call in QtConcurrent::run or a worker thread.
         */
        void process();

        /**
         * @brief updateProgressBar
         */
        void updateProgressBar();

        /**
         * @brief onUpdateMessageFileUri
         * @param filename
         * @param fileUri
         */
        void updateMessageFileUri(const QString &filename, const QString &fileUri);

        /**
         * @brief onUploadError
         * @param filePath
         * @param errorMsg
         */
        void onUploadError(const QString &filePath, const QString &errorMsg);

    private slots:

        /**
         * @brief handleApiResponse
         * @param data
         */
        void handleApiResponse(const QByteArray& data);

        /**
         * @brief processExtraction
         */
        void processExtraction();

#ifndef USE_LIBCURL
        // Internal slots for QNAM responses

        /**
         * @brief onReplyFinished
         * @param reply
         */
        void onReplyFinished(QNetworkReply *reply);
#endif

    private:

        // --- Helpers ---

        /**
         * @brief loadAudioForApi
         * @param audioPath
         * @return
         */
        QString loadAudioForApi(const QString &audioPath);

        /**
         * @brief loadImageForApi
         * @param imagePath
         * @param maxSide
         * @param quality
         * @return
         */
        QString loadImageForApi(const QString &imagePath, qint32 maxSide = 1024, qint32 quality = 80);

        /**
         * @brief loadVideoForApi
         * @param videoPath
         * @return
         */
        QString loadVideoForApi(const QString &videoPath);

        /**
         * @brief validateMediaFile
         * @param fileInfo
         * @param expectedMimePrefix
         * @return
         */
        QString validateMediaFile(const QFileInfo &fileInfo,
                                  const QString &expectedMimePrefix);

        /**
         * @brief buildJsonPayload
         * @return
         */
        QByteArray buildJsonPayload();

        /**
         * @brief canSendRequest
         * @return
         */
        bool canSendRequest();

        /**
         * @brief filterHighlights
         * @param allEvents
         * @param priorityType
         * @return
         */
        QList<SoccerEvent> filterHighlights(const QList<SoccerEvent>& allEvents,
                                            const QString& priorityType = "");

        /**
         * @brief handleSuccessfulRequest
         */
        void handleSuccessfulRequest();

        /**
         * @brief loadUsageFromDisk
         */
        void loadUsageFromDisk();

        /**
         * @brief saveUsageToDisk
         */
        void saveUsageToDisk();

        /**
         * @brief EventupdateModelLimit
         */
        void updateModelLimit();

        /**
         * @brief parseGeminiResponse
         * @param rawData
         * @return
         */
        QList<SoccerEvent> parseGeminiResponse(const QByteArray& rawData);

        /**
         * @brief setupUploadProcess
         * @param uploadFiles
         */
        void setupUploadProcess(const QMap<QString, QString> &uploadFiles);

        /**
         * @brief cleanupUploadThread
         * @param force
         */
        void cleanupUploadThread(bool force = false);

#ifdef USE_LIBCURL
        // --- Libcurl Callback ---
        /**
         * @brief writeCallback
         * @param ptr
         * @param size
         * @param nmemb
         * @param userdata
         * @return
         */
        static size_t writeCallback(void *ptr, size_t size, size_t nmemb, void *userdata);
#endif

        /**
         * @brief pendingUploads
         * @return
         */
        QMap<QString, QString> pendingUploads() const;

        /**
         * @brief waitUntilFileIsActive
         * @param fileUri
         * @return
         */
        bool waitUntilFileIsActive(const QString &fileUri);

    private: // Data members

        // --- Member Variables ---
#ifndef USE_LIBCURL
        QNetworkAccessManager * m_manager;
#endif

        // Config
        Url m_apiUrl;
        QString m_apiKey;

        QString m_contentType;

        AIModels_t m_modelCode;
        QString m_modelStr;
        Url m_siteUrl;
        QString m_siteName;

        QString m_segmentId;

        // Output
        QString m_outputFilePath;
        QByteArray m_memoryBuffer;
        QFile *m_currentFile; // Pointer to file currently being written to

        QDate m_lastRequestDate;

        qint32 m_dailyRequestCount;
        qint32 m_dailyLimit;

        MessageHistory m_messageHistory;

        QProgressBar m_uiProgressBar;

        QThread * m_uploadWorkerThread;

        UploadManager * m_uploadManagerWorker;

        qint32 m_uploadRetryCount;
        const qint32 m_maxRetries = 3;
};

Q_DECLARE_METATYPE(SoccerEvent)
Q_DECLARE_METATYPE(QList<SoccerEvent>)

Q_DECLARE_METATYPE(EventExtractionWorker *)
Q_DECLARE_OPAQUE_POINTER(EventExtractionWorker *)

#endif // EVENTEXTRACTIONWORKER_H
