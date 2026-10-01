
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMimeDatabase>

#ifdef USE_LIBCURL

extern "C" {
#include <curl/curl.h>
}

struct CurlGlobalManager {
    CurlGlobalManager() {
        // Initializes libcurl environment (Thread-safe via Q_GLOBAL_STATIC)
        curl_global_init(CURL_GLOBAL_ALL);
    }

    ~CurlGlobalManager() {
        // Automatically cleans up on App Exit
        curl_global_cleanup();
    }
};

// Declares a global static object 'curlManager'
Q_GLOBAL_STATIC(CurlGlobalManager, curlManager)

// --- RAII Helpers for LibCurl ---
// These structs ensure curl_easy_cleanup and curl_slist_free_all
// are called automatically when the variable goes out of scope.
struct CurlDeleter {
    void operator()(CURL* curl) const {
        if (curl) curl_easy_cleanup(curl);
    }
};

using CurlPtr = std::unique_ptr<CURL, CurlDeleter>;

struct CurlListDeleter {
    void operator()(struct curl_slist* slist) const {
        if (slist) curl_slist_free_all(slist);
    }
};

using CurlListPtr = std::unique_ptr<struct curl_slist, CurlListDeleter>;

struct QFileDeleter {
    void operator()(QFile* file) const {
        if (file) {
            if (file->isOpen()) file->close();
            delete file;
        }
    }
};

using QFilePtr = std::unique_ptr<QFile, QFileDeleter>;
#endif

#include <QThread>

#ifdef QT_DEBUG
#include "inc/LogUtils.h"
#endif

#include "inc/ExtractorUtils.h"
#include "inc/UploadManager.h"

/**
 * @brief The HeaderContext class
 */
struct HeaderContext {
    QString authHeader;
    QString uploadProtocol;
    QString uploadCommand;
    QString uploadHeaderContentLenght;
    QString uploadHeaderContentType;
    QString uploadOffset;
    QString contentType;
    struct uploadUrlHeaderCtx {
        QString targetHeaderName; // The header name to look for
        QString foundValue;   // Where to store the result
    } uploadlHeader;
};

// public

UploadManager::UploadManager(QObject *parent) :
    UploadManager(QString(), parent)
{
    // TODO
}

UploadManager::UploadManager(const QString &apiKey, QObject *parent) :
    QObject(parent),
    m_apiKey(apiKey),
    m_currentFile(nullptr)
#ifndef USE_LIBCURL
    ,
    m_currentReply(nullptr)
#endif
{
#ifndef USE_LIBCURL
    this->m_manager = new QNetworkAccessManager(this);
#else
    curlManager();
#endif
}

UploadManager::~UploadManager() = default;

void UploadManager::addUploadTask(const QString &filePath, const QString &displayName)
{
    this->m_queue.enqueue({filePath, displayName});
}

void UploadManager::addUploadTasks(const QMap<QString, QString> uploadTasks)
{
    for (QMap<QString, QString>::const_iterator cit = uploadTasks.constBegin();
         cit != uploadTasks.constEnd();
         cit++)
    {
        this->m_queue.enqueue(UploadTask{cit.key(), cit.value()});

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::addUploadTasks() - File %1 added to queue!")
                              .arg(cit.key()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

    }
}

AIModels_t UploadManager::modelCode() const
{
    return this->m_modelCode;
}

void UploadManager::setup(AIModels_t newModelCode, const QString &newApiKey, const Url &newApiUrl)
{
    if (newModelCode != this->m_modelCode)
    {
        this->m_modelCode = newModelCode;
    }

    if (newApiKey != this->m_apiKey)
    {
        this->m_apiUrl = newApiKey;
    }

    if (newApiUrl != this->m_apiUrl)
    {
        this->m_apiUrl = newApiUrl;
    }
}

QString UploadManager::apiKey() const
{
    return this->m_apiKey;
}

Url UploadManager::apiUrl() const
{
    return this->m_apiUrl;
}

void UploadManager::setApiUrl(const Url &newApiUrl)
{
    if (newApiUrl == this->m_apiUrl) return;

    this->m_apiUrl = newApiUrl;
}

// public slots
void UploadManager::startQueue()
{
    if (this->m_queue.isEmpty()) return;

    QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection); // Move to next in queue

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::startQueue() - Start uploading next in queue."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void UploadManager::clearAllRemoteFiles()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::startQueue() - Starting cleanup of all Gemini API files..."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->m_apiKey.isEmpty())
    {
        qWarning() << "API key is empty. Cannot clear files.";
        return;
    }

    // Prepare the Auth Header (reusable across all requests)
    QString authHeader = ExtractorUtils::authHeader(this->m_apiKey, this->m_modelCode);

#ifdef USE_LIBCURL
    CurlPtr curl(curl_easy_init());

    if (!curl)
    {
        qCritical() << "Failed to initialize cURL for cleanup.";
        return;
    }

    struct curl_slist *headersRaw = nullptr;
    headersRaw = curl_slist_append(headersRaw, qPrintable(authHeader));
    CurlListPtr headers(headersRaw); // RAII wrapper

    // Common write callback for capturing JSON responses
    auto writeCallback = [](void *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        static_cast<QByteArray*>(userdata)->append((char*)ptr, size * nmemb);
        return size * nmemb;
    };

    QString pageToken = "";
    bool hasMorePages = true;
#ifdef QT_DEBUG
    qint32 totalDeleted = 0;
#endif

    // --- PAGINATION LOOP ---
    while (hasMorePages)
    {
        // 1. Build the List URL
        QString listApiUrl = Constants::Q_QGS_GoogleGeminiFileAPI_Url->toString() + "?pageSize=100";

        if (!pageToken.isEmpty())
        {
            listApiUrl += "&pageToken=" + pageToken;
        }

        // 2. Configure cURL for the GET (List) request
        curl_easy_reset(curl.get());
        QByteArray listResponse;

        curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(listApiUrl));
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, +writeCallback);
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &listResponse);

        CURLcode res = curl_easy_perform(curl.get());

        if (res != CURLE_OK)
        {
            qCritical() << "Failed to list files:" << curl_easy_strerror(res);
            break;
        }

        // 3. Parse the JSON Response
        QJsonDocument doc = QJsonDocument::fromJson(listResponse);
        QJsonObject rootObj = doc.object();

        if (rootObj.contains("error"))
        {
            qCritical() << "API Error listing files:" << rootObj["error"].toObject()["message"].toString();
            break;
        }

        QJsonArray filesArray = rootObj.value("files").toArray();

        if (filesArray.isEmpty())
        {
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("UploadManager::startQueue() - No files found on this page. Storage is clear."),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            break;
        }

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::startQueue() - Found %1 files on current page. Deleting...")
                          .arg(filesArray.size()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        // 4. Delete every file on the current page
        for (const QJsonValue &val : qAsConst(filesArray))
        {
            QString fileName = val.toObject().value("name").toString(); // e.g., "files/abc123xyz"

            if (fileName.isEmpty()) continue;

            QString deleteUrl = QString("%1/%2").arg(Constants::Q_QGS_BaseGoogleGeminiAPI_Url->toString(), fileName);

            curl_easy_reset(curl.get());

            curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(deleteUrl));
            curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
            curl_easy_setopt(curl.get(), CURLOPT_CUSTOMREQUEST, "DELETE");

            res = curl_easy_perform(curl.get());

            if (res == CURLE_OK)
            {
                long http_code = 0;
                curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

                if (http_code == 200)
                {
#ifdef QT_DEBUG
                    totalDeleted++;

                    LogUtils::safeLog(QString("UploadManager::startQueue() - Deleted %1 (%2 total)")
                                          .arg(fileName)
                                      .arg(totalDeleted),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                } else {
                    qWarning() << "Failed to delete" << fileName << "- HTTP Code:" << http_code;
                }
            } else {
                qCritical() << "cURL error while deleting" << fileName << ":" << curl_easy_strerror(res);
            }
        }

        // 5. Check if we need to fetch another page
        if (rootObj.contains("nextPageToken"))
        {
            pageToken = rootObj.value("nextPageToken").toString();

            if (pageToken.isEmpty())
            {
                hasMorePages = false;
            } else {
                qDebug() << "";
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("UploadManager::startQueue() - Fetching next page of files..."),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        } else {
            hasMorePages = false; // No more tokens, we're done
        }
    }
#endif

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::startQueue() - Cleanup complete. Successfully deleted %1 files from Gemini API.")
                          .arg(totalDeleted),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

// private slots

void UploadManager::processNext()
{
    if (this->m_queue.isEmpty())
    {
        emit allUploadsCompleted();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::startQueue() - All uploads completed."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::startQueue() - Uploading next in queue."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QString errMsg;

    if (this->m_apiUrl.isEmpty())
    {
        errMsg = "Error: no Model Url";

        emit errorOccurred("", errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1")
                          .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    this->m_currentTask = this->m_queue.dequeue();

    if (this->m_currentTask.filePath.isEmpty())
    {
        errMsg = "Error: There is no file defined: ";

        emit errorOccurred(this->m_currentTask.filePath, errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1!")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

    QFilePtr fileGuard(new QFile(this->m_currentTask.filePath));

    if (!fileGuard->exists())
    {
        errMsg = "Error: File does not exist: " + fileGuard->fileName();

        emit errorOccurred(this->m_currentTask.filePath, errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1!")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

    this->m_currentFile = fileGuard.get();

    if (!this->m_currentFile->open(QIODevice::ReadOnly))
    {
        errMsg = "Error: Could not open file: " + this->m_currentTask.displayName;

        emit errorOccurred(this->m_currentTask.displayName, "");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1!")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection); // Move to next in queue

        return;
    }

    QString authHeader = ExtractorUtils::authHeader(this->m_apiKey, this->m_modelCode);

    if (authHeader.isEmpty())
    {
        errMsg = "Error: authorization header is empty";

        emit errorOccurred("", errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

#ifdef QT_DEBUG
    errMsg = QString("Authoriation header gotten.");

    LogUtils::safeLog(QString("UploadManager::startQueue() - %1")
                          .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Step 1: Initiate Resumable Upload
    HeaderContext headerContext = this->getHeaderContext();

    QJsonObject fileObj;
    fileObj["display_name"] = this->m_currentTask.displayName;
    QJsonObject root;
    root["file"] = fileObj;

#ifndef USE_LIBCURL
    // ASYNCHRONOUS PATH
    // Release ownership from the unique_ptr because the QFile must live
    // until onUploadFinished is called.
    this->m_currentFile = fileGuard.release();

    QNetworkRequest request(this->m_apiUrl.toString());

    request.setRawHeader(headerContext.authHeader, this->m_apiKey);
    request.setRawHeader(headerContext.uploadProtocol, "resumable");
    request.setRawHeader(headerContext.uploadCommand, "start");
    request.setRawHeader(headerContext.uploadHeaderContentLenght, QByteArray::number(this->m_currentFile->size()));
    request.setRawHeader(headerContext.uploadHeaderContentType,  *URLContentTypeAudioMP4_Str);
    request.setHeader(QNetworkRequest::ContentTypeHeader, *URLContentTypeJason_Str);

    QNetworkReply *reply = this->m_manager->post(request, QJsonDocument(root).toJson());

    connect(reply,
            &QNetworkReply::finished,
            this,
            &UploadManager::onInitFinished);
#else

    QByteArray jsonPayload = QJsonDocument(root).toJson(QJsonDocument::Compact);

#ifdef QT_DEBUG
    errMsg = QString("jsonPayload created.");

    LogUtils::safeLog(QString("UploadManager::startQueue() - %1")
                          .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // QString resumableUrl;

    // auto headerCallback = [](char *buffer, size_t size, size_t nitems, void *userdata) -> size_t {
    //     auto *ctx = static_cast<HeaderContext*>(userdata);
    //     QString header = QString::fromLatin1(buffer, size * nitems);
    //     if (header.startsWith(ctx->uploadlHeader.targetHeaderName, Qt::CaseInsensitive)) {
    //         ctx->uploadlHeader.foundValue = header.mid(18).trimmed();
    //     }

    //     return size * nitems;
    // };

    CurlPtr curl(curl_easy_init());

    if ( ! curl )
    {
        errMsg = "Error: Failed to init cURL";

        emit errorOccurred("", errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    struct curl_slist *initHeadersRaw = nullptr;

    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1").arg(headerContext.authHeader)));
    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1: resumable").arg(headerContext.uploadProtocol)));
    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1: start").arg(headerContext.uploadCommand)));
    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1: %2").arg(headerContext.uploadHeaderContentLenght, QString::number(this->m_currentFile->size()))));

    QMimeDatabase db;
    QString fileActualMimeType = db.mimeTypeForFile(this->m_currentTask.filePath).name();

    if (fileActualMimeType.isEmpty())
    {
        fileActualMimeType = *Constants::Q_QGS_URLContentTypeOctetStream_Str; // Safe fallback
    }

    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1: %2").arg(headerContext.uploadHeaderContentType, fileActualMimeType)));
    initHeadersRaw = curl_slist_append(initHeadersRaw, qPrintable(QString("%1: %2").arg(headerContext.contentType, *Constants::Q_QGS_URLContentTypeJason_Str)));

    CurlListPtr initHeaders(initHeadersRaw); // RAII wrap

    curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(this->m_apiUrl.toString()));
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, initHeaders.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, jsonPayload.constData());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, jsonPayload.size());

    // --- PHASE 1 SAFE LIMITS & CONNECTION FRESHNESS ---
    // Prevent Google GFE from throttling reused TCP sockets
    curl_easy_setopt(curl.get(), CURLOPT_FORBID_REUSE, 1L); // Force a new connection
    curl_easy_setopt(curl.get(), CURLOPT_FRESH_CONNECT, 1L);

    // Timeout quickly if the handshake hangs
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 10L);


    QByteArray responseHeaders;
    curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, UploadManager::writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &responseHeaders);

    QByteArray responseBody;
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, UploadManager::writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);

    CURLcode res = curl_easy_perform(curl.get());

#ifdef QT_DEBUG
    errMsg = QString("Curl executed.");

    LogUtils::safeLog(QString("UploadManager::startQueue() - %1")
                          .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (res != CURLE_OK)
    {
        if (this->m_currentTask.retryCount < 3)
        {
            this->m_currentTask.retryCount++;

            qWarning() << "Phase 1 Error for" << this->m_currentTask.displayName << ":" << curl_easy_strerror(res) << "- Retrying attempt" << this->m_currentTask.retryCount;

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("UploadManager::processNext() - Phase 1 Error for %1: %2 - Retrying attempt %3")
                                  .arg(this->m_currentTask.displayName, curl_easy_strerror(res))
                              .arg(this->m_currentTask.retryCount),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            // Put the task back at the front of the queue
            this->m_queue.prepend(this->m_currentTask);

            // Backoff sleep before retrying the connection
            QThread::msleep(2000);

        }
        else
        {
            emit errorOccurred(this->m_currentTask.displayName, QString("Phase 1 cURL Error: ") + curl_easy_strerror(res));

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("UploadManager::processNext() - Failed to start upload for %1 after 3 attempts. \n %2")
                                  .arg(this->m_currentTask.displayName, curl_easy_strerror(res)),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

        QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection);

        return;
    }

    QString uploadUrl;

    // const QList<QByteArray> lines = responseHeaders.split('\n');

    // for (const QByteArray& line : qAsConst(lines))
    // {
    //     if (line.toLower().startsWith("location:"))
    //     {
    //         uploadUrl = QString::fromLatin1(line.mid(9).trimmed());
    //         break;
    //     }
    // }

    qint32 idx = responseHeaders.indexOf(headerContext.uploadlHeader.targetHeaderName.toUtf8(), Qt::CaseInsensitive);

    if (idx != -1)
    {
        qint32 endIdx = responseHeaders.indexOf('\n', idx);
        if (endIdx == -1) endIdx = responseHeaders.length(); // Failsafe

        QByteArray line = responseHeaders.mid(idx, endIdx - idx);
        // Split by colon and take the rest to handle arbitrary spacing
        QString lineStr = QString::fromLatin1(line);

        qint32 colonIdx = lineStr.indexOf(':');
        if (colonIdx != -1)
        {
            uploadUrl = lineStr.mid(colonIdx + 1).trimmed();
        }
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::startQueue() - Upload phase 1 - Extracted Upload URL: %1 \n\n %2")
                          .arg(uploadUrl, QString(responseHeaders)),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (uploadUrl.isEmpty())
    {
        emit errorOccurred(this->m_currentTask.displayName, "Phase 1 Failed to extract Upload URL from headers:\n" + QString(responseHeaders));

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::startQueue() - Phase 1 Failed to extract Upload URL from headers:\n %1")
                              .arg(QString(responseHeaders)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection);
        return;
    }

    // --- STEP 2: Actual File Upload (PUT) ---
    curl_easy_reset(curl.get()); // Clears everything! Reuse the same CurlPtr

    // Reset the file cursor to the absolute beginning
    this->m_currentFile->seek(0);

    struct curl_slist *uploadHeadersRaw = nullptr;

    uploadHeadersRaw = curl_slist_append(uploadHeadersRaw, qPrintable(QString("%1: 0").arg(headerContext.uploadOffset)));
    uploadHeadersRaw = curl_slist_append(uploadHeadersRaw, qPrintable(QString("%1: upload, finalize").arg(headerContext.uploadCommand)));

    CurlListPtr uploadHeaders(uploadHeadersRaw); // RAII wrap

    curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(uploadUrl));
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, uploadHeaders.get());

    auto readCallback = [](char *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        qint64 bytesRead = static_cast<QFile*>(userdata)->read(ptr, size * nmemb);

        if (bytesRead < 0) return CURL_READFUNC_ABORT; // Gracefully abort if disk read fails

        return static_cast<size_t>(bytesRead);
    };

    // Re-set Read logic
    curl_easy_setopt(curl.get(), CURLOPT_READDATA, this->m_currentFile);
    curl_easy_setopt(curl.get(), CURLOPT_READFUNCTION, +readCallback);
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)this->m_currentFile->size());

    // Re-set Progress logic
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, UploadManager::progressCallback);
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, this);
    curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);

    // --- PHASE 2 STREAMING LIMITS & CONNECTION FRESHNESS ---
    curl_easy_setopt(curl.get(), CURLOPT_FORBID_REUSE, 1L); // Close connection nicely when done
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 0L); // Infinite overall time for large files
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, 10L); // 10 bytes/sec
    curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, 30L); // Abort if stalled for 30s

    // Re-set these after reset!
    QByteArray finalResponseBody;
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, UploadManager::writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &finalResponseBody);

    QByteArray finalResponseHeaders;
    curl_easy_setopt(curl.get(), CURLOPT_HEADERFUNCTION, UploadManager::writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_HEADERDATA, &finalResponseHeaders);

    res = curl_easy_perform(curl.get());


    if (res == CURLE_OK)
    {
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::processNext() - UploadManager Phase 2 Completed.\nHTTP Code: %1\nBody: %2")
                              .arg(http_code).arg(QString(finalResponseBody)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        if (http_code >= 200 && http_code < 300)
        {
            QJsonDocument doc = QJsonDocument::fromJson(finalResponseBody);
            const QJsonObject jObj = doc.object();
            QJsonObject fileObj = jObj.value("file").toObject();

            if (fileObj.contains("uri"))
            {
                QString fileUri = fileObj.value("uri").toString();

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("UploadManager::processNext() - Emitting uploadFinished for: %1 -> %2")
                                      .arg(this->m_currentTask.displayName, fileUri),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
                emit uploadFinished(this->m_currentTask.displayName, fileUri);

            } else {
                emit errorOccurred(this->m_currentTask.displayName, "Upload succeeded but no URI returned: " + QString(finalResponseBody));

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("UploadManager::processNext() - Upload succeeded but no URI returned"),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        } else {
            emit errorOccurred(this->m_currentTask.displayName, QString("Phase 2 HTTP Error %1: %2").arg(http_code).arg(QString(finalResponseBody)));

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("UploadManager::processNext() - Phase 2 HTTP Error %1: %2")
                                  .arg(http_code)
                                  .arg(QString(finalResponseBody)),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

    } else {
        emit errorOccurred(this->m_currentTask.displayName, "Phase 2 cURL Error: " + QString(curl_easy_strerror(res)));

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::startQueue() - Curl perform error: %1 --- %2")
                              .arg(this->m_currentTask.displayName, curl_easy_strerror(res)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
#endif

    // Be polite to the API. Sleep for 500ms before starting the next video.
    QThread::msleep(500);

    QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection); // Move to next in queue
}

void UploadManager::onInitFinished()
{
#ifndef USE_LIBCURL
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError)
    {
        emit errorOccurred(this->m_currentTask.displayName, "Init Failed: " + reply->errorString());

        reply->deleteLater();

        QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection); // Move to next in queue

        return;
    }

    // Capture the 'Location' header for the actual upload
    Url uploadUrl = reply->header(QNetworkRequest::LocationHeader).toUrl();

    reply->deleteLater();

    // Step 2: Perform the actual PUT upload
    QNetworkRequest putRequest(uploadUrl);
    putRequest.setRawHeader("X-Goog-Upload-Offset", "0");
    putRequest.setRawHeader("X-Goog-Upload-Command", "upload, finalize");

    this->m_currentReply = m_manager->put(putRequest, m_currentFile);

    connect(this->m_currentReply,
            &QNetworkReply::uploadProgress,
            this,
            &UploadManager::onUploadProgress);

    connect(this->m_currentReply,
            &QNetworkReply::finished,
            this,
            &UploadManager::onUploadFinished);
#endif
}

void UploadManager::onUploadFinished()
{
#ifndef USE_LIBCURL
    if (this->m_currentReply->error() == QNetworkReply::NoError)
    {
        QJsonDocument doc = QJsonDocument::fromJson(this->m_currentReply->readAll());
        QString fileUri = doc.object()["file"].toObject()["uri"].toString();

        emit uploadFinished(this->m_currentTask.displayName, fileUri);

    } else {
        emit errorOccurred(this->m_currentTask.displayName, this->m_currentReply->errorString());
    }

    this->m_currentReply->deleteLater();

    if (this->m_currentFile)
    {
        this->m_currentFile->close();
        delete this->m_currentFile;
        this->m_currentFile = nullptr;
    }

    QMetaObject::invokeMethod(this, "processNext", Qt::QueuedConnection); // Move to next in queue
#endif
}

void UploadManager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0)
    {
        int progress = static_cast<int>((bytesSent * 100) / bytesTotal);

        emit progressChanged(this->m_currentTask.displayName, progress);
    }
}

#ifdef USE_LIBCURL

void UploadManager::updateProgress(qint32 percentage)
{
    emit progressChanged(this->m_currentTask.displayName, percentage);
}

#endif

HeaderContext UploadManager::getHeaderContext()
{
    HeaderContext headerContext;

    if (this->m_apiKey.isEmpty())
    {
        if ((this->m_apiKey = ExtractorUtils::getAPIKey(this->m_modelCode)).isEmpty())
        {
            emit errorOccurred("", "Error: no API key");

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtraction::process() - API key is empty!"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return headerContext;
        }
    }

#pragma clang diagnostic ignored "-Wswitch"
    switch (this->m_modelCode)
    {
        case AIModels_t::Google_Gemini_API_20FlashLite:
        case AIModels_t::Google_Gemini_API_20FlashLite001:
        case AIModels_t::Google_Gemini_API_20Flash:
        case AIModels_t::Google_Gemini_API_20Flash001:
        case AIModels_t::Google_Gemini_API_25Flash:
        case AIModels_t::Google_Gemini_API_25FlashLite:
        case AIModels_t::Google_Gemini_API_25Pro:
        case AIModels_t::Google_Gemini_API_31FlashLite:
        case AIModels_t::Google_Gemini_API_31FlashLitePreview:
        case AIModels_t::Google_Gemini_API_FlashLiteLatest:
        case AIModels_t::Google_Gemini_API_FlashLatest:
        case AIModels_t::Google_Gemini_API_3FlashPreview:
        case AIModels_t::Google_Gemini_API_3ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreview:
        case AIModels_t::Google_Gemini_API_31ProPreviewCustomTools:
        case AIModels_t::Google_Gemini_API_ProLatest:
        {
            if (this->m_apiUrl.isEmpty())
            {
                this->m_apiUrl = *Constants::Q_QGS_GoogleGeminiFileAPI_Url;
            }

#ifndef USE_LIBCURL
            headerContext.authHeader = Utils::headerTemplate(HeadersType::GoogleAuthorisation).replace("<KEY>", "");
#else
            headerContext.authHeader = ExtractorUtils::headerTemplate(HeaderType_t::GoogleAuthorisation).replace("<KEY>", this->m_apiKey);
#endif

            headerContext.uploadProtocol = "X-Goog-Upload-Protocol";
            headerContext.uploadCommand = "X-Goog-Upload-Command";
            headerContext.uploadHeaderContentLenght = "X-Goog-Upload-Header-Content-Length";
            headerContext.uploadHeaderContentType = "X-Goog-Upload-Header-Content-Type";
            headerContext.contentType = "Content-Type";
            headerContext.uploadOffset = "X-Goog-Upload-Offset";

            headerContext.uploadlHeader.targetHeaderName = "x-goog-upload-url";

            break;
        }

        case AIModels_t::Google_HuggingFace_API:
        case AIModels_t::GPT_4o_Mini_API:
        case AIModels_t::OpenAI_Google_API:
        case AIModels_t::OpenRouter_API_FreeModel:
        case AIModels_t::OpenRouter_API_Deepseekv2:
        case AIModels_t::OpenRouter_API_Deepseekv3Base:
        case AIModels_t::OpenRouter_API_DeepSeekDeepSeekV3:
        case AIModels_t::OpenRouter_API_GoogleGemini2_0FlashExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemini2_5ProExperimental:
        case AIModels_t::OpenRouter_API_GoogleGemma3_12B:
        case AIModels_t::OpenRouter_API_GoogleGemma3_27B:
        case AIModels_t::OpenRouter_API_GPT_4o_Mini:
        case AIModels_t::OpenRouter_API_MetaLlama4Maverick:
        case AIModels_t::OpenRouter_API_MetaLlama4Scout:
        case AIModels_t::OpenRouter_API_MicrosoftMAIDSR1:
        case AIModels_t::OpenRouter_API_MistralMistralNemo:
        case AIModels_t::OpenRouter_API_Qwen3235BA22B:
        {
#ifndef USE_LIBCURL
            headerContext.authHeader = Utils::headerTemplate(HeadersType::Authorization).replace("<KEY>", "");
#else
            headerContext.authHeader = ExtractorUtils::headerTemplate(HeaderType_t::Authorization).replace("<KEY>", this->m_apiKey);
#endif

            // TODO

            break;
        }

        case AIModels_t::UserDefined_API:
        case AIModels_t::None:
        {
            break;
        }
    }

    return headerContext;
}

#ifdef USE_LIBCURL

bool UploadManager::checkFileState(const QString &fileUri)
{
    QString errMsg;

    // 1. Extract file ID from URI (e.g., https://.../files/abc-123 -> abc-123)
    QString fileName = fileUri.split('/').last();

    if (this->m_apiUrl.isEmpty())
    {
        errMsg = "Error: no Model Url";

        emit errorOccurred("", errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::checkFileState(...) - %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return false;
    }

    QString authHeader = ExtractorUtils::authHeader(this->m_apiKey, this->m_modelCode);

    if (authHeader.isEmpty())
    {
        errMsg = "Error: authorization header is empty";

        emit errorOccurred("", errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("UploadManager::checkFileState() - %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return false;
    }

    qint32 attempts = 0;

    const qint32 maxAttempts = 30; // 30 attempts * 2 seconds = 1 minute timeout

    QElapsedTimer timer;
    timer.start();

    CurlPtr curl(curl_easy_init());
    if (!curl) return false;

    struct curl_slist *rawHeaders = nullptr;
    rawHeaders = curl_slist_append(rawHeaders, qPrintable(authHeader));
    CurlListPtr headers(rawHeaders);

    curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(QString("%1/%2").arg(this->m_apiUrl.toString(), fileName)));
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());

    // Setup response capture
    auto writeCallback = [](void *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        static_cast<QByteArray*>(userdata)->append((char*)ptr, size * nmemb);
        return size * nmemb;
    };

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, writeCallback);

    while (attempts < maxAttempts)
    {
        QByteArray responseBuffer;

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBuffer);

        CURLcode res = curl_easy_perform(curl.get());

        if (res == CURLE_OK)
        {
            QJsonDocument doc = QJsonDocument::fromJson(responseBuffer);
            QJsonObject obj = doc.object();

            if (obj.contains("error"))
            {
                QString apiError = obj["error"].toObject()["message"].toString();

                emit errorOccurred(fileName, "Google API Error: " + apiError);

                return false;
            }

            QString state = obj["state"].toString();

            if (state == "ACTIVE")
            {
                errMsg = QString("File [%1] became ACTIVE in %2 ms after %3 attempts.")
                             .arg(fileName).arg(timer.elapsed()).arg(attempts + 1);

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("UploadManager::checkFileState(...) - %1")
                                      .arg(errMsg),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                return true;

            } else if (state == "FAILED")
            {
                errMsg = QString("Google processing failed for file: %1").arg(fileName);

                qCritical() << errMsg;

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("UploadManager::checkFileState(...) - %1")
                                      .arg(errMsg),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                return false;
            }
        } else {
            // Log cURL errors but don't abort immediately; it might be a temporary network blip
            qWarning() << "cURL error during state check:" << curl_easy_strerror(res);
        }

        // If we reach here, it's still "PROCESSING"
        attempts++;

        // This is safe because this logic runs in a worker thread.
        // It prevents the next file in the queue from starting until this one is ready.
        QThread::sleep(2);
    }

    errMsg = QString("Timeout waiting for ACTIVE status for file: %1").arg(fileName);

    qWarning() << errMsg;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("UploadManager::checkFileState(...) - %1")
                          .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return false; // Timeout
}

size_t UploadManager::writeCallback(void *contents, size_t size, size_t nmemb, QByteArray *userp)
{
    size_t totalSize = size * nmemb;

    userp->append(static_cast<const char*>(contents), totalSize);

    return totalSize;
}

// Helper for progress tracking
int UploadManager::progressCallback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    Q_UNUSED(dltotal)
    Q_UNUSED(dlnow)

    auto* manager = static_cast<UploadManager*>(clientp);

    if (ultotal > 0)
    {
        int progress = static_cast<int>((ulnow * 100) / ultotal);
        // Using meta-method to ensure thread-safety if run in a thread

        QMetaObject::invokeMethod(manager, "updateProgress", Q_ARG(int, progress));
    }
    return 0;
}
#endif
