
#include <QDebug>
#include <QDate>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimeZone>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QMimeType>
#include <QProcessEnvironment>
#include <QThread>
#include <QTimer>
#include <QSettings>

#ifndef USE_LIBCURL
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#else
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

struct FileRAII {
    QFile* file;
    explicit FileRAII(const QString& path, QIODevice::OpenMode mode) :
        file(new QFile(path))
    {
        if (!file->open(mode))
        {
            qWarning() << "RAII: Failed to open file:" << path;
        }
    }

    ~FileRAII()
    {
        if (file)
        {
            if (file->isOpen()){
                file->close();
            }
            delete file;
            file = nullptr;
            qDebug() << "RAII: File closed and memory freed.";
        }
    }

    // Safety: Prevent copying to avoid double-deletion crashes
    FileRAII(const FileRAII&) = delete;
    FileRAII& operator=(const FileRAII&) = delete;

    bool exists(const QString &path)
    {
        return QFile::exists(path);
    }

    void remove(const QString &path)
    {
        QFile::remove(path);
    }

    // Helper to check status
    bool isReady() const
    {
        return file && file->isOpen();
    }
};

using QFilePtr = QScopedPointer<FileRAII>;

#endif

#include "Url.h"
using namespace utils;

#include "inc/LogUtils.h"
#include "inc/ExtractorUtils.h"
#include "inc/EventExtractionWorker.h"

EventExtractionWorker::EventExtractionWorker(QObject *parent) :
    QObject(parent),
#ifndef USE_LIBCURL
    // Initialize QNAM
    m_manager(new QNetworkAccessManager(this))
#endif
    m_modelCode(AIModels_t::None),
    m_segmentId(QString()),
    m_currentFile(nullptr),
    m_dailyLimit(20), // If using Gemini 2.5 Flash and a subscription daily limit, this limit is greater.
    m_uploadWorkerThread(nullptr),
    m_uploadManagerWorker(nullptr),
    m_uploadRetryCount(0)
{
    qRegisterMetaType<SoccerEvent>("SoccerEvent");
    qRegisterMetaType<QList<SoccerEvent>>("QList<SoccerEvent>");

    this->m_messageHistory.clear();
    this->loadUsageFromDisk();

    // Start by updating the UI with loaded data
    // this->updateProgressBar();

#ifdef USE_LIBCURL
    curlManager();
#endif
}

EventExtractionWorker::~EventExtractionWorker() = default;

bool EventExtractionWorker::operator ==(const EventExtractionWorker &right) const
{
    if ((this->m_apiUrl != right.m_apiUrl) ||
        (this->m_apiKey != right.m_apiKey) ||
        (this->m_contentType != right.m_contentType) ||
        (this->m_modelCode != right.m_modelCode) ||
        (this->m_modelStr != right.m_modelStr) ||
        (this->m_siteUrl != right.m_siteUrl) ||
        (this->m_siteName != right.m_siteName) ||
        (this->m_segmentId != right.m_segmentId) ||
        (this->m_outputFilePath != right.m_outputFilePath) ||
        (this->m_memoryBuffer != right.m_memoryBuffer) ||
        (this->m_currentFile != right.m_currentFile) ||
        (this->m_lastRequestDate != right.m_lastRequestDate) ||
        (this->m_dailyRequestCount != right.m_dailyRequestCount) ||
        (this->m_dailyLimit != right.m_dailyLimit) ||
        (this->m_messageHistory != right.m_messageHistory))
    {
        return false;
    }

    return true;
}

bool EventExtractionWorker::operator !=(const EventExtractionWorker &right) const
{
    return !(*this == right);
}

bool EventExtractionWorker::operator==(const EventExtractionWorker *ptr) const
{
    return (this == ptr);
}

bool EventExtractionWorker::operator!=(const EventExtractionWorker *ptr) const
{
    return (this != ptr);
}

QString EventExtractionWorker::apiKey() const
{
    return this->m_apiKey;
}

void EventExtractionWorker::setApiKey(const QString &newKey)
{
    if (newKey == this->m_apiKey) return;

    this->m_apiKey = newKey;
}

Url &EventExtractionWorker::apiUrl()
{
    return this->m_apiUrl;
}

void EventExtractionWorker::setApiUrl(const Url &newApiUrl)
{
    if (newApiUrl == this->m_apiUrl) return;

    this->m_apiUrl = newApiUrl;
}

QString EventExtractionWorker::modelStr() const
{
    return m_modelStr;
}

void EventExtractionWorker::setModelStr(const QString &newModelStr)
{
    m_modelStr = newModelStr;
}

const QString &EventExtractionWorker::contentType() const
{
    return this->m_contentType;
}

void EventExtractionWorker::setContentType(ContentMimeType_e newContentType)
{
    QString newCntTypeStr;

    if (ContentTypeMap.contains(newContentType)) {
        const auto& cntType = ContentTypeMap[newContentType];
        newCntTypeStr = cntType;
    }

    if (newCntTypeStr == this->m_contentType)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setContentType(...) - Content Type: No change !"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

    this->m_contentType = newCntTypeStr;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::setContentType(...) - Content Type: %1 !").arg(this->m_contentType),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void EventExtractionWorker::setModel(AIModels_t newModel,
                                     const QString &customModelString,
                                     const QString &customAPIUrl,
                                     const QString &customAPIKey)
{
    if (this->m_modelCode == newModel) return;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::setModel(...) - Setting model!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    Url apiUrl;

    if (AIModelMap.contains(newModel))
    {
        const auto& cfg = AIModelMap[newModel];
        this->m_modelStr = cfg.name;
        apiUrl.setUrl(cfg.url.toString().arg(this->m_modelStr));
        this->setApiUrl(apiUrl);
        this->setApiKey(cfg.key);
    } else if (newModel == AIModels_t::UserDefined_API) {
        this->m_modelStr = customModelString;
        apiUrl.setUrl(customAPIUrl);
        this->setApiUrl(apiUrl);
        this->setApiKey(customAPIKey);
    }

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractionWorker::setModel(const AIModels_t) - %1: %2 / %3!")
                              .arg(this->m_modelStr, this->apiUrl().toString(), this->apiKey()),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

    this->m_modelCode = newModel;
}

void EventExtractionWorker::setSiteInfo(const Url &newSiteUrl, const QString &newSiteName)
{
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setSiteInfo(const Url &, const QString &) - Entering!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

    if (newSiteUrl != this->m_siteUrl)
    {
        this->m_apiUrl = newSiteUrl;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setSiteInfo(const Url &, const QString &) - New site Url!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    if (newSiteName != this->m_siteName)
    {
        this->m_siteName = newSiteName;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setSiteInfo(const Url &, const QString &) - New site name!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

QString EventExtractionWorker::segmentId() const
{
    return this->m_segmentId;
}

void EventExtractionWorker::setSegmentId(const QString &segmentId)
{
    if (segmentId == this->m_segmentId) return;

    this->m_segmentId = segmentId;
}

void EventExtractionWorker::setSystemPrompt(const QString &prompt)
{
    // Ensure system prompt is always first
    if (! this->m_messageHistory.isEmpty() &&
        this->m_messageHistory.at(0).role() == Message::RoleType::System)
    {
        this->m_messageHistory.at(0).setRole(Message::RoleType::System);
        this->m_messageHistory.at(0).setContent(prompt);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setSystemPrompt(...) - System prompt set: at 0."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

    } else {

        Message msg(Message::RoleType::System, prompt);
        this->m_messageHistory.prepend(msg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::setSystemPrompt(...) - System prompt set: prepended."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

void EventExtractionWorker::addUserMessage(const QString &content)
{
    Message msg(Message::RoleType::User, content);
    this->m_messageHistory.append(msg);
}

void EventExtractionWorker::addUserMessageWithAudio(const QString &text, const QString &audioPath)
{
    QFileInfo fileInfo(audioPath);
    QString fileMimeType = this->validateMediaFile(fileInfo, "audio");

    if (fileMimeType.isEmpty()) return;

    Message msg;

    if (fileInfo.size() <= *Constants::Q_QGS_DEFAULT_FILE_SIZE)
    {
        // Process the image using the helper
        // 1024x1024 is the "sweet spot" for Gemini/OpenAI vision models
        QString cleanBase64 = this->loadAudioForApi(fileInfo.absoluteFilePath());

        if (cleanBase64.isEmpty())
        {
            // Handle error (maybe fallback to text only?)
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithAudio(...) - Skipping audio due to load error."),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return;
        }

        // Create the message using your struct
        // The constructor we discussed earlier sets hasMedia = true automatically
        //    Message msg("user", text, cleanBase64);

        msg.setMediaBase64(cleanBase64);

    } else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithAudio() - File is too large for inline sending (%1MB).Limit is 20MB")
                              .arg(fileInfo.size() / (1024*1024)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);

        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithAudio() - 'File API' upload method will be used for this file."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    msg.setRole(Message::RoleType::User);
    msg.setContent(text);
    msg.setFilePath(fileInfo.absoluteFilePath());
    msg.setMimeType(fileMimeType);

    this->m_messageHistory.append(msg);
}

void EventExtractionWorker::addUserMessageWithImage(const QString &text, const QString &imagePath)
{
    QFileInfo fileInfo(imagePath);

    QString fileMimeType = this->validateMediaFile(fileInfo, "image");

    if (fileMimeType.isEmpty()) return;

    Message msg;

    if (fileInfo.size() <= *Constants::Q_QGS_DEFAULT_FILE_SIZE)
    {
        // Process the image using the helper
        // 1024x1024 is the "sweet spot" for Gemini/OpenAI vision models
        QString cleanBase64 = this->loadImageForApi(fileInfo.absoluteFilePath(), 1024, 85);

        if (cleanBase64.isEmpty())
        {
            // Handle error (maybe fallback to text only?)
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithImage(...) - Skipping image due to load error."),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return;
        }

        // Create the message using your struct
        // The constructor we discussed earlier sets hasMedia = true automatically
        //    Message msg("user", text, cleanBase64);

        msg.setMediaBase64(cleanBase64);

    } else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithImage() - File is too large for inline sending (%1 MB).Limit is 20MB")
                              .arg(fileInfo.size() / (1024*1024)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);

        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithImage() - 'File API' upload method will be used for this file."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    msg.setRole(Message::RoleType::User);
    msg.setContent(text);
    msg.setFilePath(fileInfo.absoluteFilePath());
    msg.setMimeType(fileMimeType);

    this->m_messageHistory.append(msg);
}

void EventExtractionWorker::addUserMessageWithVideo(const QString &text, const QString &videoPath)
{
    QFileInfo fileInfo(videoPath);

    QString fileMimeType = this->validateMediaFile(fileInfo, "video");

    if (fileMimeType.isEmpty()) return;

    Message msg;

    // 2. Check Size Limit (Critical for Inline Video)
    // Gemini Inline Limit is 20 MB (20 * 1024 * 1024 bytes)
    if (fileInfo.size() <= *Constants::Q_QGS_DEFAULT_FILE_SIZE)
    {
        QString cleanBase64 = this->loadVideoForApi(fileInfo.absoluteFilePath());

        if (!cleanBase64.isEmpty())
        {
            msg.setMediaBase64(cleanBase64);
        }
    }
    else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithVideo() - File is too large for inline sending (%1 MB).Limit is 20MB")
                          .arg(fileInfo.size() / (1024*1024)),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);

        LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithVideo() - 'File API' upload method will be used for this file."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    msg.setRole(Message::RoleType::User);
    msg.setContent(text);
    msg.setFilePath(fileInfo.absoluteFilePath());
    msg.setMimeType(fileMimeType);

    this->m_messageHistory.append(msg);
}

void EventExtractionWorker::addUserMessageWithVideosList(const QString &text, const QStringList &videosPaths)
{
    if (videosPaths.isEmpty()) return;

    QMimeDatabase db;
    QMimeType fileMimeType;

    for (const auto & videoPath : qAsConst(videosPaths))
    {
        if (videoPath.isEmpty()) continue;

        QFileInfo fileInfo(videoPath);

        if ( fileInfo.exists() )
        {
            QString absPath = fileInfo.absoluteFilePath();
            fileMimeType = db.mimeTypeForFile(absPath);

            if (fileMimeType.name().startsWith("video"))
            {
                Message msg(Message::RoleType::User,
                            text,
                            QString(),
                            fileMimeType.name(),
                            absPath);

                // Apply the 20 MB logic
                if (fileInfo.size() <= *Constants::Q_QGS_DEFAULT_FILE_SIZE)
                {
                    QString cleanBase64 = this->loadVideoForApi(absPath);

                    if (!cleanBase64.isEmpty())
                    {
                        msg.setMediaBase64(cleanBase64);
                    }
                }

                this->m_messageHistory.append(msg);
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::addUserMessageWithVideosList(...) - Video %1 (%2) added to list!")
                                  .arg(absPath, fileMimeType.name()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        }
    }
}

void EventExtractionWorker::addMessageHistory(const MessageHistory &newMsgHistory)
{
    if (newMsgHistory.isEmpty()) return;

    this->m_messageHistory = newMsgHistory;
}

void EventExtractionWorker::clearMessages()
{
    this->m_messageHistory.clear();
}

void EventExtractionWorker::setOutputFile(const QString &newFilePath)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::setOutputFile(...) - Setting output file up!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (newFilePath == this->m_outputFilePath) return;

    this->m_outputFilePath = newFilePath;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::setOutputFile(...) - Output file set: %1!")
                      .arg(this->m_outputFilePath),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

// public slots
void EventExtractionWorker::process()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::process() --- Entering"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Guard against empty history
    if (this->m_messageHistory.isEmpty())
    {
        QString errMsg = QString("Message history is empty!");
        emit errorOccurred(errMsg);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::process() --- Message history is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::process() --- Message history has %1 messages")
                          .arg(this->m_messageHistory.size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 2. Identify pending uploads (Task Discovery)
    QMap<QString, QString> uploadFiles = this->pendingUploads();

    if (uploadFiles.isEmpty())
    {
        // Data is perfectly clean. Every media file has a URI or Base64.
        this->m_uploadRetryCount = 0; // Reset the retry counter

        // Everything is already uploaded (or no media), jump to extraction
        QMetaObject::invokeMethod(this, "processExtraction", Qt::QueuedConnection);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::process() --- There are %1 file to process")
                              .arg(uploadFiles.size()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    // 3. Check Retry Limits
    if (this->m_uploadRetryCount >= this->m_maxRetries)
    {
        emit errorOccurred("Upload failed after multiple attempts.");
        return;
    }

    // 4. Setup Execution (The "Plumbing")
    // If thread is already running, wait for it to finish or abort it
    if (this->m_uploadWorkerThread && this->m_uploadWorkerThread->isRunning())
    {
        return; // Prevent overlapping attempts
    }

    this->setupUploadProcess(uploadFiles);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::process() --- Upload process set."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void EventExtractionWorker::updateProgressBar()
{
    // 1. Safety check to ensure the UI pointer is valid
    // (Assuming you have a pointer to the progress bar or a signal-slot connection)
    if ( ! this->m_uiProgressBar.isEnabled() ) this->m_uiProgressBar.setEnabled(true);

    // 2. Update the Range and Value
    this->m_uiProgressBar.setRange(0, m_dailyLimit);
    this->m_uiProgressBar.setValue(m_dailyRequestCount);

    // 3. Set Custom Text Format: Shows "12 / 20" instead of percentage
    this->m_uiProgressBar.setFormat(QString("%1 / %2").arg(m_dailyRequestCount).arg(m_dailyLimit));

    // 4. Dynamic Color Logic (Thresholds)
    float ratio = (float)m_dailyRequestCount / m_dailyLimit;
    QString colorStyle;

    if (ratio >= 0.95f)
    {
        // Red: Critical (1 request left or zero)
        colorStyle = "QProgressBar::chunk { background-color: #f44336; }";
    } else if (ratio >= 0.75f) {
        // Yellow: Warning
        colorStyle = "QProgressBar::chunk { background-color: #ffeb3b; }";
    } else {
        // Green: Good
        colorStyle = "QProgressBar::chunk { background-color: #4caf50; }";
    }

    // Apply the style to the bar
    this->m_uiProgressBar.setStyleSheet(colorStyle);
}

void EventExtractionWorker::updateMessageFileUri(const QString &filename, const QString &fileUri)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri() - SIGNAL RECEIVED for %1")
                          .arg(filename),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif


    if (filename.isEmpty() || fileUri.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri - Received empty data: Path=%1, URI=%2")
                              .arg(filename, fileUri),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

    if (this->m_messageHistory.updateMessageFileUri(filename, fileUri))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri() - Updated URI successfully!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    } else {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri - Received upload finished for unknown file: File name=%1")
                              .arg(filename),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri() - Leaving!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

// Example of an error slot if UploadManager supports it
void EventExtractionWorker::onUploadError(const QString &filename, const QString &errorMsg)
{
    qCritical() << "Upload failed for" << filename << ":" << errorMsg;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::updateMessageFileUri - Upload failed for %1: %2")
                          .arg(filename, errorMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
    // We don't decrement m_updatedMessages here,
    // which signals to our 'process()' logic that a retry is needed.
}

// private slots
void EventExtractionWorker::handleApiResponse(const QByteArray& data)
{
    QList<SoccerEvent> allEvents;

    // 1. Convert raw JSON to QList
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
            allEvents = this->parseGeminiResponse(data);
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
            // TODO
            break;
        }

        case AIModels_t::UserDefined_API:
        case AIModels_t::None:
        {
            break;
        }
    }

    // 2. Filter for only the important highlights
    QList<SoccerEvent> highlights = this->filterHighlights(allEvents, "");

    // 3. Send to UI
    emit eventsProcessed(highlights);
}

void EventExtractionWorker::processExtraction()
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Content Type: %1 !").arg(this->m_contentType),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
    // Quota Check (Custom logic added)
    if (!this->canSendRequest())
    {
        emit errorOccurred("Daily API limit reached. Resets at Midnight PT.");
        emit finished(false);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Daily API limit reached. Resets at Midnight PT."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return;
    }

    if (this->apiUrl().isEmpty())
    {
        emit errorOccurred("Error: no Model Url");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Error: no Model Url!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    // Validation: Ensure all media messages are actually ready
    const auto &messages = this->m_messageHistory.messages();

    for (const auto &msg : qAsConst(messages))
    {
        if (msg.hasMedia())
        {
            // Check if we have EITHER base64 OR a remote URI
            if (msg.mediaBase64().isEmpty() && msg.fileUri().isEmpty())
            {
                QString err = QString("Validation Error: Media message missing data for %1").arg(msg.filePath());

                emit errorOccurred(err);

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - %1")
                                      .arg(err),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                return;
            }
        }

        if (!msg.fileUri().isEmpty() && msg.mimeType().startsWith("video/"))
        {
            if (!this->waitUntilFileIsActive(msg.fileUri()))
            {
                emit errorOccurred("Timeout or error waiting for Google to process video: " + msg.filePath());

                emit finished(false);

                return;
            }
        }
    }

    QString authHeader = ExtractorUtils::authHeader(this->m_apiKey, this->m_modelCode);

    if (authHeader.isEmpty())
    {
        emit errorOccurred("Error: authorization header is empty");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Error: authorization header is empty!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    emit started();

    QByteArray payload = this->buildJsonPayload();
    this->m_memoryBuffer.clear();

//#ifdef QT_DEBUG
//    LogUtils::safeLog(QString("EventExtractionWorker::process() - Json payload: %1!")
//                      .arg(QString(payload.constData())),
//                      __FILE__,
//                      __LINE__,
//                      Q_FUNC_INFO);
//#endif

#ifdef USE_LIBCURL

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Setting Curl!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    CurlPtr curl(curl_easy_init());

    if ( ! curl )
    {
        emit errorOccurred("Error: Failed to init cURL");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Error: Failed to init cURL"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    // 1. Prepare Data
    struct curl_slist *headersRaw = nullptr;
    headersRaw = curl_slist_append(headersRaw, authHeader.toUtf8().constData());

    if ( ! this->m_contentType.isEmpty() )
    {
        QString contHeader = ExtractorUtils::headerTemplate(HeaderType_t::Content_Type);
        contHeader.replace("<CONTENT_TYPE>", this->m_contentType);
        headersRaw = curl_slist_append(headersRaw, contHeader.toUtf8().constData());

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Content type: %1")
                              .arg(this->m_contentType),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    if ( ! this->m_siteUrl.isEmpty())
    {
        QString ref = ExtractorUtils::headerTemplate(HeaderType_t::HttpReferer);
        ref.replace("<SITE_URL>", this->m_siteUrl.toString());
        headersRaw = curl_slist_append(headersRaw, ref.toUtf8().constData());

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Site: %1")
                              .arg(this->m_siteUrl.toString()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    if ( ! this->m_siteName.isEmpty())
    {
        QString title = ExtractorUtils::headerTemplate(HeaderType_t::X_Title);
        title.replace("X-Title: ", this->m_siteName);
        headersRaw = curl_slist_append(headersRaw, title.toUtf8().constData());

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Site: %1")
                              .arg(this->m_siteName),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // Wrap headers in RAII to ensure they are freed
    CurlListPtr headers(headersRaw);

    // 2. Prepare Output Strategy
    this->m_currentFile = nullptr;
    QFilePtr fileGuard; // Local RAII guard

    if ( ! this->m_outputFilePath.isEmpty() )
    {
        // If file exists, remove it to start fresh
        if (QFile::exists(this->m_outputFilePath)) QFile::remove(this->m_outputFilePath);

        // Create the file and wrap it immediately
        fileGuard.reset(new FileRAII(this->m_outputFilePath, QIODevice::WriteOnly));

        if ( ! fileGuard->isReady() )
        {
            emit errorOccurred("Error: Could not open output file: " + this->m_outputFilePath);

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Error: Could not open output file: %1!")
                                  .arg(this->m_outputFilePath),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            return;
        }

        this->m_currentFile = fileGuard->file;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Output file: %1")
                              .arg(this->m_outputFilePath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - AI Model Url: %1")
                          .arg(this->apiUrl().toString()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 3. Configure cURL
    curl_easy_setopt(curl.get(), CURLOPT_URL, this->apiUrl().toString().toUtf8().constData());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);

    // --- ADD LOW SPEED LIMITS HERE ---
    // This acts as a network-level watchdog.
    // If the transfer speed is < 10 bytes/sec for 30 consecutive seconds, cURL aborts.
    // This may cause the request to be aborted.
    // curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, 10L);
    // curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, 30L);

    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, payload.constData());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, payload.size());

    // Set Callback
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, EventExtractionWorker::writeCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, this); // Pass 'this' instance

    // Optional: Follow redirects and set timeout
    curl_easy_setopt(curl.get(), CURLOPT_FOLLOWLOCATION, 1L);

    // Two minutes timeout for multi-video processing.
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 600L);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - [PIPELINE UPDATE] -> Payload successfully sent to Gemini! Waiting for AI "
                              "analysis... (This may take several minutes)\n"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 4. Perform
    CURLcode res = curl_easy_perform(curl.get());

    // 5. Cleanup File: it is done in EventExtractionWorker::writeCallback function.

    // 6. Handle Result
    if (res != CURLE_OK)
    {
        QString err = QString("cURL Error: %1").arg(curl_easy_strerror(res));

        qCritical() << err;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - %1!")
                              .arg(err),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit errorOccurred(err);

        emit finished(false);

    } else {
        long http_code = 0;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code >= 200 && http_code < 300)
        {
            if (this->m_outputFilePath.isEmpty())
            {
                emit responseReceived(this->segmentId(), QString::fromUtf8(this->m_memoryBuffer));

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Memory Mode: Output file path is empty. Emitting buffer."),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

            } else {
                emit fileSaved(this->m_outputFilePath);

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - File is ok!"),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }

            emit finished(true);
        } else {

            if (http_code == 429)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - 429 Quota Exceeded in libcurl process."),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }

            QString err = QString("HTTP Error Code: %1").arg(http_code);
            // Even on error, APIs often return JSON details, check buffer/file

            if (!this->m_memoryBuffer.isEmpty())
            {
                qCritical() << "Gemini API Error Detail Output:" << QString::fromUtf8(this->m_memoryBuffer);
            }

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - %1!")
                                  .arg(err),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            emit errorOccurred(err);
            emit finished(false);
        }
    }
#else
    QNetworkRequest request(this->m_apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, this->m_contentType);

    // Dynamic Header Setup
    bool isGoogle = (this->m_modelCode >= AIModels_t::Google_Gemini_API_20Flash &&
                     this->m_modelCode <= AIModels_t::Google_Gemini_API_30Pro);

    if (isGoogle)
    {
        request.setRawHeader("x-goog-api-key", key.toUtf8());
    } else {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(key).toUtf8());
    }

    if ( ! this->m_siteUrl.isEmpty() )
    {
        QString ref = Utils::headerTemplate(HeadersType::HttpReferer);
        ref.replace("<SITE_URL>", "");
        request.setRawHeader(ref.toUtf8().constData, this->m_siteUrl.toString().toUtf8().constData());
    }

    if ( ! this->m_siteName.isEmpty() )
    {
        QString title = Utils::headerTemplate(HeadersType::X_Title);
        title.replace("X-Title: ", "");
        request.setRawHeader(title.toUtf8().constData, this->m_siteName.toString().toUtf8().constData());
    }

    QNetworkReply *reply = this->m_manager->post(request, payload);

    connect(reply,
            &QNetworkReply::uploadProgress,
            this,
            &EventExtractionWorker::progressUpdated);

    connect(this->m_manager,
            &QNetworkAccessManager::finished,
            this,
            &EventExtractionWorker::onReplyFinished);

#endif

    this->m_currentFile = nullptr;
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionWorker::processExtraction() - Processed!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

#ifndef USE_LIBCURL
// 2. In your onReplyFinished (for Gemini 2.5 Flash)
void EventExtractionWorker::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        QString err("Error %1: %2").arg(code).arg(reply->errorString());
        emit errorOccurred(err);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::process() - %1")
                          .arg(err),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit finished(false);
        return;
    }

    this->handleSuccessfulRequest();

    QByteArray responseData = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);

    // Identify schema to extract the text content
    QString extractedText;
    QJsonObject root = doc.object();

    if (root.contains("choices")) // OpenRouter / OpenAI / etc.
    {
        extractedText = root["choices"].toArray().first().toObject()["message"].toObject()["content"].toString();

    } else if (root.contains("candidates")) // Gemini
    {
        extractedText = root["candidates"].toArray().first().toObject()["content"].toObject()["parts"].toArray().first().toObject()["text"].toString();
    }

    emit responseReceived(this->this->m_segmentId, extractedText);
    emit finished(true);

    this->updateProgressBar();
}
#endif

// private

QString EventExtractionWorker::loadAudioForApi(const QString &audioPath)
{
    QFileInfo fileInfo(audioPath);
    if (!fileInfo.exists())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadAudioForApi() - Error: Audio file does not exist."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    QFile file(fileInfo.absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadAudioForApi() - Error: Failed to load audio at %1!")
                              .arg(fileInfo.absoluteFilePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    QByteArray data = file.readAll();
    return QString::fromLatin1(data.toBase64());
}

QString EventExtractionWorker::loadImageForApi(const QString &imagePath, qint32 maxSide, qint32 quality)
{
    QFileInfo fileInfo(imagePath);

    if (!fileInfo.exists())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadImageForApi() - Error: Image file does not exist."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    QImage img(fileInfo.absoluteFilePath());

    if (img.isNull())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadImageForApi() - Error: Failed to load image at %1!")
                          .arg(fileInfo.absoluteFilePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    // 1. Resize if the image is too large
    // (Large images waste tokens and upload time without improving AI accuracy much)
    if (img.width() > maxSide || img.height() > maxSide)
    {
        img = img.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    // 2. Convert to efficient format (RGB888 avoids issues with indexed PNGs)
    if (img.format() != QImage::Format_RGB888)
    {
        img = img.convertToFormat(QImage::Format_RGB888);
    }

    // 3. Compress to JPEG inside a memory buffer
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);

    // "JPG" format and 'quality' setting drastically reduce payload size
    img.save(&buffer, "JPG", quality);

    // 4. Return standard Base64
    return QString::fromLatin1(byteArray.toBase64());
}

QString EventExtractionWorker::loadVideoForApi(const QString &videoPath)
{
    QFileInfo fileInfo(videoPath);

    // 1. Check Existence
    if (!fileInfo.exists())
    {
        qDebug() << "EventExtractionWorker::loadVideoForApi() - Error: Video file does not exist:" << videoPath;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadVideoForApi() - Error: Video file does not exist: %1!")
                          .arg(fileInfo.absoluteFilePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    // 2. Load Data
    QFile file(fileInfo.absoluteFilePath());

    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "EventExtractionWorker::loadVideoForApi() - Error: Could not open video file.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::loadVideoForApi() - Error: Could not open video file."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    QByteArray videoData = file.readAll();
    file.close();

    // 4. Encode to Base64
    // Note: This does NOT add the "data:video/mp4;base64," header.
    // The Gemini API expects raw base64 in the 'data' field.
    return QString::fromLatin1(videoData.toBase64());
}

QString EventExtractionWorker::validateMediaFile(const QFileInfo &fileInfo,
                                              const QString &expectedMimePrefix)
{
    if (!fileInfo.exists())
    {
        qDebug() << "Error: File does not exist:" << fileInfo.absoluteFilePath();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::validateMediaFile(...) - Error: File does not exist: %1")
                          .arg(fileInfo.absoluteFilePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return QString();
    }

    QString mimeStr = QMimeDatabase().mimeTypeForFile(fileInfo.absoluteFilePath()).name();

    if (!mimeStr.startsWith(expectedMimePrefix))
    {
        qDebug() << "Error: File is not" << expectedMimePrefix << ":" << fileInfo.absoluteFilePath();
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::validateMediaFile(...) - Error: File is not %1 : %2")
                              .arg(expectedMimePrefix, fileInfo.absoluteFilePath()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return QString();
    }

    return mimeStr;
}

QByteArray EventExtractionWorker::buildJsonPayload()
{
    QJsonObject root;
    const auto &messages = this->m_messageHistory.messages();

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
            QJsonArray contentsArr;
            QJsonArray sysParts;

            for (const auto &msg : qAsConst(messages))
            {
                QJsonObject partObj = msg.toJsonObject(Message::Schema::Gemini);

                if (msg.role() == Message::RoleType::System) // 1. Add System Instruction (Persona/Rules)
                {
                    sysParts.append(partObj);
                } else {
                    contentsArr.append(partObj);
                }
            }

            // Insert System Instruction if parts exist
            if ( ! sysParts.isEmpty() )
            {
                QJsonObject sysObj;
                sysObj[QStringLiteral("parts")] = sysParts;
                root[QStringLiteral("system_instruction")] = sysObj;
            }

            if ( ! contentsArr.isEmpty())
            {
                root[QStringLiteral("contents")] = contentsArr;
            }

            // --- ADDED: Safety Settings ---
            QJsonArray safetySettings;
            auto addSafety = [&](const QString &category, const QString &threshold)
            {
                QJsonObject setting;
                setting.insert("category", category);
                setting.insert("threshold", threshold);
                safetySettings.append(setting);
            };

            // Use BLOCK_ONLY_HIGH to be more permissive for technical/sports analysis
            addSafety(QStringLiteral("HARM_CATEGORY_HARASSMENT"),        QStringLiteral("BLOCK_ONLY_HIGH"));
            addSafety(QStringLiteral("HARM_CATEGORY_HATE_SPEECH"),       QStringLiteral("BLOCK_ONLY_HIGH"));
            addSafety(QStringLiteral("HARM_CATEGORY_SEXUALLY_EXPLICIT"), QStringLiteral("BLOCK_ONLY_HIGH"));
            addSafety(QStringLiteral("HARM_CATEGORY_DANGEROUS_CONTENT"), QStringLiteral("BLOCK_ONLY_HIGH"));

            root[QStringLiteral("safetySettings")] = safetySettings;

            // Optional: Add Generation Config for temperature/top_p and media_resolution.
            QJsonObject genConfig;

            /*
             * Enum Constant                    Description
             * MEDIA_RESOLUTION_UNSPECIFIED     Default value. The system decides the optimal resolution automatically.
             * MEDIA_RESOLUTION_LOW             Low resolution parameter configuration (typically optimized for rapid
             *                                  text extraction or lower token usage).
             * MEDIA_RESOLUTION_FULL            High-fidelity resolution tracking (best for intricate details like
             *                                  spotting small text, numbers, or specific soccer match actions).
             */

            switch (this->m_modelCode)
            {
                case AIModels_t::Google_Gemini_API_20FlashLite:
                case AIModels_t::Google_Gemini_API_20FlashLite001:
                case AIModels_t::Google_Gemini_API_20Flash:
                case AIModels_t::Google_Gemini_API_20Flash001:
                {
                    // "inputTokenLimit": 1048576
                    genConfig[QStringLiteral("maxOutputTokens")] = 8192; // Force the absolute maximum response size
                    genConfig[QStringLiteral("media_resolution")] = QStringLiteral("MEDIA_RESOLUTION_LOW");

                    break;
                }

                case AIModels_t::Google_Gemini_API_25Pro:
                {
                    // "inputTokenLimit": 1048576
                    genConfig[QStringLiteral("maxOutputTokens")] = 65536; // Force the absolute maximum response size
                    genConfig[QStringLiteral("media_resolution")] = QStringLiteral("MEDIA_RESOLUTION_FULL");

                    break;
                }

                case AIModels_t::Google_Gemini_API_25Flash:
                case AIModels_t::Google_Gemini_API_25FlashLite:
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

#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractionWorker::buildJsonPayload()() - Gemini model!"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif

                    // "inputTokenLimit": 1048576
                    genConfig[QStringLiteral("maxOutputTokens")] = 65536; // Force the absolute maximum response size
                    genConfig[QStringLiteral("media_resolution")] = QStringLiteral("MEDIA_RESOLUTION_LOW");

                    break;
                }
            }

            genConfig[QStringLiteral("temperature")] = 0.7;
            root[QStringLiteral("generationConfig")] = genConfig;

            break;
        }

        case AIModels_t::Google_HuggingFace_API:
        {
            // Hugging Face TGI/Inference endpoints use OpenAI-compatible "messages"
            // but strictness varies by model (e.g., Idefics vs Llama).
            root[QStringLiteral("model")] = this->m_modelStr;

            QJsonArray messagesArr;

            for (const auto &msg : qAsConst(messages))
            {
                QJsonObject mObj = msg.toJsonObject(Message::Schema::Google_HuggingFace);

                messagesArr.append(mObj);
            }

            root[QStringLiteral("messages")] = messagesArr;

            // Standard Generation Config
            root[QStringLiteral("max_tokens")] = 1024;
            root[QStringLiteral("temperature")] = 0.7;

            break;
        }

        case AIModels_t::GPT_4o_Mini_API:
        {
            root[QStringLiteral("model")] = this->m_modelStr;
            root[QStringLiteral("temperature")] = 0.7;

            QJsonArray messagesArr;

            for (const auto &msg : qAsConst(messages))
            {
                QJsonObject mObj = msg.toJsonObject(Message::Schema::GPT_4o_Mini);
                messagesArr.append(mObj);
            }

            root[QStringLiteral("messages")] = messagesArr;

            break;
        }

        case AIModels_t::OpenAI_Google_API:
        {
            root[QStringLiteral("model")] = this->m_modelStr;
            root[QStringLiteral("temperature")] = 0.7;

            QJsonArray messagesArr;

            for (const auto &msg : qAsConst(messages))
            {
                QJsonObject mObj = msg.toJsonObject(Message::Schema::OpenAI_Google);
                messagesArr.append(mObj);
            }

            root[QStringLiteral("messages")] = messagesArr;

            break;
        }

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
            root[QStringLiteral("model")] = this->m_modelStr;

            QJsonArray messagesArr;

            for (const auto &msg : qAsConst(messages))
            {
                QJsonObject mObj = msg.toJsonObject(Message::Schema::OpenRouter);

                messagesArr.append(mObj);
            }

            root[QStringLiteral("messages")] = messagesArr;

            // 4. Optional: OpenRouter-specific or Standard Params
            // OpenRouter respects these standard OpenAI parameters
            /*
            root["temperature"] = 0.7; // Creativity (0.0 to 1.0/2.0)
            root["top_p"] = 1.0;       // Nucleus sampling
            root["max_tokens"] = 1000; // Limit output length

            // OpenRouter Specific: "transforms" (optional)
            // Automatically compress prompts to save credits
            // root["transforms"] = QJsonArray() << "middle-out";
            */

            break;
        }

        case AIModels_t::UserDefined_API:
        {
            // TODO
        }
    }

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact);
}

// Call this before sending a request
bool EventExtractionWorker::canSendRequest()
{
    // 1. Get current date in Pacific Time (Google's reset timezone)
    QDateTime nowPT = QDateTime::currentDateTime().toTimeZone(QTimeZone("America/Los_Angeles"));
    QDate todayPT = nowPT.date();

    // 2. Check if the day has changed since the last request
    if (todayPT > this->m_lastRequestDate)
    {
        this->m_dailyRequestCount = 0;
        this->m_lastRequestDate = todayPT;
        this->saveUsageToDisk(); // Save the reset state
    }

    return m_dailyRequestCount < m_dailyLimit;
}

/*
 *
 * Usage Examples
 * Reel                 TypeCode Call                           Result
 * ===================================================================================================
 * Full Highlights      filterHighlights(list, "");             Shows all goals, fouls, and cards.
 * Elite Reel           filterHighlights(list, "Compound");     Shows only complex multi-stage
 *                                                              events (the "best" clips).
 * Quick Clips          filterHighlights(list, "Atomic");       Shows simple, single-action moments.
 */
QList<SoccerEvent> EventExtractionWorker::filterHighlights(const QList<SoccerEvent>& allEvents,
                                                           const QString& priorityType)
{
    QList<SoccerEvent> highlights;

    // Keywords for critical game moments
    QStringList keywords;
    keywords << *Constants::Q_QGS_CAT_GOAL << *Constants::Q_QGS_CAT_LAST_MINUTE_GOAL << *Constants::Q_QGS_CAT_PENALTY << *Constants::Q_QGS_CAT_RED_CARD <<
        *Constants::Q_QGS_CAT_RED_CARD_INCIDENT << *Constants::Q_QGS_CAT_YELLOW_CARD << *Constants::Q_QGS_CAT_FOUL << *Constants::Q_QGS_CAT_PASS <<
        *Constants::Q_QGS_CAT_CORNER_KICK << *Constants::Q_QGS_CAT_SHOT << *Constants::Q_QGS_CAT_FREE_KICK << *Constants::Q_QGS_CAT_HIGH_PASS <<
        *Constants::Q_QGS_CAT_SUBSTITUTION << *Constants::Q_QGS_CAT_THROW_IN << *Constants::Q_QGS_CAT_PLAYER_SUCCESSFUL_TACKLE <<
        *Constants::Q_QGS_CAT_GOAL_KICK << *Constants::Q_QGS_CAT_PENALTY_KICK << *Constants::Q_QGS_CAT_SAVE << *Constants::Q_QGS_CAT_DRAMATIC_SAVE <<
        *Constants::Q_QGS_CAT_PLAYER_BLOCK << *Constants::Q_QGS_CAT_OUT << *Constants::Q_QGS_CAT_CROSS << *Constants::Q_QGS_CAT_OFFSIDE_CALL <<
        *Constants::Q_QGS_CAT_BALL << *Constants::Q_QGS_CAT_INJURY_TIMEOUT << *Constants::Q_QGS_CAT_DRIVE << *Constants::Q_QGS_CAT_HEADER <<
        *Constants::Q_QGS_CAT_HAT_TRICK << *Constants::Q_QGS_CAT_INJURY << *Constants::Q_QGS_CAT_CELEBRATION_MOMENT << *Constants::Q_QGS_CAT_VIRAL_MOMENT <<
        *Constants::Q_QGS_CAT_TEAM_HUDDLE << *Constants::Q_QGS_CAT_MANAGER_REACTION;

    for (const SoccerEvent& event : allEvents)
    {
        // 1. If priorityType is provided (e.g., "Compound"), skip events that don't match
        if (!priorityType.isEmpty() && event.type != priorityType)
        {
            continue;
        }

        // 2. Perform Keyword Search
        for (const QString& word : qAsConst(keywords))
        {
            // Reusing your manual case-insensitive logic via the QString wrapper
            if (event.description.contains(word, Qt::CaseInsensitive))
            {
                highlights.append(event);
                break;
            }
        }
    }

    return highlights;
}

// Update this in your onReplyFinished (or libcurl loop)
void EventExtractionWorker::handleSuccessfulRequest()
{
    this->m_dailyRequestCount++;
    this->saveUsageToDisk(); // Save immediately so we don't lose the count

    // Calculate percentage for progress bar
    float progress = (float)this->m_dailyRequestCount / m_dailyLimit * 100;

    emit percentageUpdated(progress);
    emit usageUpdated(this->m_dailyRequestCount, this->m_dailyLimit);
}

void EventExtractionWorker::loadUsageFromDisk()
{
    QSettings settings("Rios' Coorp", "EventExtractorApp");

    this->updateModelLimit();

    settings.beginGroup("Usage_" + this->m_modelStr);
    this->m_dailyRequestCount = settings.value("dailyCount", 0).toInt();

    // Load the date; default to a very old date if not found
    QString dateStr = settings.value("lastDate", "2000-01-01").toString();
    this->m_lastRequestDate = QDate::fromString(dateStr, Qt::ISODate);
    settings.endGroup();

    // Reset check: If today is a new day (Pacific Time), reset the count
    QDateTime nowPT = QDateTime::currentDateTime().toTimeZone(QTimeZone("America/Los_Angeles"));

    if (nowPT.date() > this->m_lastRequestDate)
    {
        this->m_dailyRequestCount = 0;
        this->m_lastRequestDate = nowPT.date();
        this->saveUsageToDisk();
    }
}

void EventExtractionWorker::saveUsageToDisk()
{
    // These strings identify your app in the Windows Registry / macOS Plist
    QSettings settings("Rios' Coorp", "EventExtractorApp");

    settings.beginGroup("Usage_");
    settings.setValue("dailyCount", this->m_dailyRequestCount);
    settings.setValue("lastDate", this->m_lastRequestDate.toString(Qt::ISODate));
    settings.endGroup();

    // Forces the OS to write the file immediately
    settings.sync();
}

void EventExtractionWorker::updateModelLimit()
{
    if (this->m_modelStr.contains("flash"))
    {
        this->m_dailyLimit = 1500; // Flash-Lite usually has a much higher free quota
    } else if (m_modelStr.contains("pro") || m_modelStr.contains("ultra")) {

        this->m_dailyLimit = 50;   // Pro models often have tighter free limits
    } else {

        this->m_dailyLimit = 20;   // Default safety limit for standard Flash
    }
}

QList<SoccerEvent> EventExtractionWorker::parseGeminiResponse(const QByteArray& rawData)
{
    QList<SoccerEvent> extractedEvents;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(rawData, &error);

    if (error.error != QJsonParseError::NoError)
    {
        qDebug() << "JSON Parse Error:" << error.errorString();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("QList<SoccerEvent> EventExtractionWorker::parseGeminiResponse(...) - JSON Parse Error: %1")
                          .arg(error.errorString()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return extractedEvents;
    }

    QJsonArray eventsArray = doc.array();

    for (const QJsonValue& value : qAsConst(eventsArray))
    {
        QJsonObject obj = value.toObject();

        SoccerEvent event;
        // 1. Convert "HH:MM:SS" string to absolute seconds
        event.timestamp = ExtractorUtils::toSeconds(obj["start_time"].toString());

        // 2. Extract the description
        event.description = obj["original_text"].toString();

        // 3. Extract the 'type' field with a default fallback
        QString typeVal = obj["type"].toString();
        event.type = typeVal.isEmpty() ? "Atomic" : typeVal;

        extractedEvents.append(event);
    }

    return extractedEvents;
}

void EventExtractionWorker::setupUploadProcess(const QMap<QString, QString> &uploadFiles)
{
    QString errMsg;

    // For uploading a set of files to File API
    this->cleanupUploadThread(false);

    // Initialize Thread and Worker
    if (!this->m_uploadWorkerThread)
    {
        this->m_uploadWorkerThread = new QThread;
    }

    if (!this->m_uploadManagerWorker)
    {
        this->m_uploadManagerWorker = new UploadManager(this->m_apiKey);
    }

    this->m_uploadManagerWorker->clearAllRemoteFiles();

    this->m_uploadManagerWorker->moveToThread(this->m_uploadWorkerThread);
    this->m_uploadManagerWorker->setup(this->m_modelCode, this->m_apiKey, *Constants::Q_QGS_GoogleGeminiFileAPI_Url);
    this->m_uploadManagerWorker->addUploadTasks(uploadFiles);

#ifdef QT_DEBUG
    errMsg = QString("Upload Manager set and upload tasks added");
    LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() --- %1")
                          .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Watchdog Timer (The "Zombie" Connection Killer)
    // We set a 5-minute total limit for the batch. Adjust based on video sizes.
    QTimer* watchdog = new QTimer(this);
    watchdog->setSingleShot(true);
    const qint32 timeoutMs = 1800000;

    // --- Connections ---

    // Start processing when thread starts
    QObject::connect(this->m_uploadWorkerThread,
                     &QThread::started,
                     this->m_uploadManagerWorker,
                     &UploadManager::startQueue);

    // Progress
    QObject::connect(this->m_uploadManagerWorker,
                     &UploadManager::uploadFinished,
                     this,
                     &EventExtractionWorker::updateMessageFileUri,
                     Qt::QueuedConnection);

    // Handle Completion (Success or Partial Failure)
    QObject::connect(this->m_uploadManagerWorker,
                     &UploadManager::allUploadsCompleted,
                     this,
                     [this, watchdog]() {
                         watchdog->stop();
                         watchdog->deleteLater();

                         QMap<QString, QString> missingFiles = this->pendingUploads();

                         qDebug() << "All uploads completed signal received. Pending messages:" << missingFiles.size();
#ifdef QT_DEBUG
                         LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() - UploadManager Phase 3 - Queue finished. Pending URI assignments: %1")
                                               .arg(missingFiles.size()),
                                           __FILE__,
                                           __LINE__,
                                           Q_FUNC_INFO);
#endif

                         // Logic check: Did all files get a URI?
                         if (missingFiles.isEmpty())
                         {
                             this->m_uploadRetryCount = 0; // Success! Reset retry counter

                             this->cleanupUploadThread(false); // Clean up only on success

#ifdef QT_DEBUG
                             LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() - All URIs assigned successfully! Launching processExtraction..."),
                                               __FILE__,
                                               __LINE__,
                                               Q_FUNC_INFO);
#endif

                             QMetaObject::invokeMethod(this, "processExtraction", Qt::QueuedConnection);
                         } else {

                             this->m_uploadRetryCount++;

                             qWarning() << "Some uploads pending. Retrying... Count:" << this->m_uploadRetryCount;

#ifdef QT_DEBUG
                             LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() --- Some uploads pending. Retrying... Count:: %1")
                                                   .arg(this->m_uploadRetryCount),
                                               __FILE__,
                                               __LINE__,
                                               Q_FUNC_INFO);
#endif
                             this->cleanupUploadThread(false); // Clean up before retry

                             QTimer::singleShot(5000, this, &EventExtractionWorker::process);
                         }
                     });

    // Handle Watchdog Timeout
    QObject::connect(watchdog,
                     &QTimer::timeout,
                     this,
                     [this, watchdog]() {
                         qCritical() << "Upload Manager Watchdog triggered: Timeout.";

#ifdef QT_DEBUG
                         LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() --- Upload Manager Watchdog triggered: Timeout."),
                                           __FILE__,
                                           __LINE__,
                                           Q_FUNC_INFO);
#endif

                         this->cleanupUploadThread(true);

                         watchdog->deleteLater();
                         emit errorOccurred("Network timeout during video upload. Please try again.");
#ifdef QT_DEBUG
                         LogUtils::safeLog(QString("EventExtractionWorker::setupUploadProcess() --- Network timeout during video upload. Please try again."),
                                           __FILE__,
                                           __LINE__,
                                           Q_FUNC_INFO);
#endif
                     });

    // Increment retries
    this->m_uploadRetryCount++;

    // Execution
    this->m_uploadWorkerThread->start();
    watchdog->start(timeoutMs);
}

void EventExtractionWorker::cleanupUploadThread(bool force)
{
    if (this->m_uploadWorkerThread)
    {
        if (this->m_uploadWorkerThread->isRunning())
        {
            this->m_uploadWorkerThread->quit();

            if (force) this->m_uploadWorkerThread->terminate();

            this->m_uploadWorkerThread->wait();
        }

        // Safely delete the worker object (it was moved to the thread,
        // but since the thread is dead, we can delete it from here)
        if (this->m_uploadManagerWorker)
        {
            delete this->m_uploadManagerWorker;
            this->m_uploadManagerWorker = nullptr;
        }

        // Safely delete the thread
        delete this->m_uploadWorkerThread;
        this->m_uploadWorkerThread = nullptr;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::cleanupUploadThread() --- Upload thread and worker fully destroyed."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

size_t EventExtractionWorker::writeCallback(void *contentsPtr, size_t size, size_t nmemb, void *userdata)
{
    size_t realSize = size * nmemb;
    EventExtractionWorker *worker = static_cast<EventExtractionWorker*>(userdata);

    if ( ! worker ) return 0; // Error

    if (worker->m_currentFile && worker->m_currentFile->isOpen() && worker->m_currentFile->isWritable())
    {
        // Write to File
        qint64 written = worker->m_currentFile->write(static_cast<const char*>(contentsPtr), realSize);

        if (written != static_cast<qint64>(realSize))
        {
            qCritical() << "RAII: Write error. Disk full or file locked.";
            return 0; // Returning 0 tells cURL to abort the transfer
        }
    } else {
        // Write to Memory
        worker->m_memoryBuffer.append(static_cast<const char*>(contentsPtr), realSize);
    }

    return realSize;
}

QMap<QString, QString> EventExtractionWorker::pendingUploads() const
{
    QMap<QString, QString> pendingFiles;

    const auto &msgList = this->m_messageHistory.messages();

    for (const auto &msg : qAsConst(msgList))
    {
        if (msg.hasMedia() &&
            msg.fileUri().isEmpty() &&
            msg.mediaBase64().isEmpty() &&
            !msg.filePath().isEmpty())
        {
            QFileInfo fileInfo(msg.filePath());

            if (fileInfo.exists())
            {
                pendingFiles.insert(fileInfo.absoluteFilePath(), fileInfo.fileName());

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::pendingUploads() --- There is/are %1 file(s) to process")
                                      .arg(pendingFiles.size()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }
        }
    }

    return pendingFiles;
}

bool EventExtractionWorker::waitUntilFileIsActive(const QString &fileUri)
{
    if (fileUri.isEmpty()) return false;

    QString authHeader = ExtractorUtils::authHeader(this->m_apiKey, this->m_modelCode);

    if (authHeader.isEmpty()) return false;

    qint32 attempts = 0;
    const qint32 maxAttempts = 60; // 2 minutes

#ifdef USE_LIBCURL
    CurlPtr curl(curl_easy_init());

    if (!curl) return false;

    struct curl_slist *rawHeaders = nullptr;
    rawHeaders = curl_slist_append(rawHeaders, qPrintable(authHeader));
    CurlListPtr headers(rawHeaders);

    auto checkWriteCallback = [](void *ptr, size_t size, size_t nmemb, void *userdata) -> size_t {
        static_cast<QByteArray*>(userdata)->append((char*)ptr, size * nmemb);
        return size * nmemb;
    };

    while (attempts < maxAttempts)
    {
        curl_easy_reset(curl.get());
        curl_easy_setopt(curl.get(), CURLOPT_URL, qPrintable(fileUri));
        curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
        curl_easy_setopt(curl.get(), CURLOPT_HTTPGET, 1L);

        curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, +checkWriteCallback);

        QByteArray responseBuffer;
        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBuffer);

        CURLcode res = curl_easy_perform(curl.get());

        if (res == CURLE_OK)
        {
            long http_code = 0;

            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &http_code);

            QJsonObject obj = QJsonDocument::fromJson(responseBuffer).object();

            if (http_code >= 200 && http_code < 300)
            {
                QString state = obj["state"].toString();

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::waitUntilFileIsActive(...) --- Polling %1 - State: %2")
                                      .arg(fileUri, state),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                if (state == "ACTIVE") return true;

                if (state == "FAILED")
                {
                    qCritical() << "Google failed to process video:" << fileUri;

#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("EventExtractionWorker::waitUntilFileIsActive(...) --- Google failed to process video: %1")
                                          .arg(fileUri),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                    return false;
                }
            }
            else if (http_code == 500 || http_code == 502 || http_code == 503 || http_code == 504 || http_code == 429)
            {
                // Transient backend error — log and allow loop to retry
                qWarning() << "[File API] Transient HTTP" << http_code << "encountered while polling. Retrying in 3s...";

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::waitUntilFileIsActive(...) --- Transient HTTP %1 encountered while polling. Retrying in 3s...")
                                      .arg(http_code),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                QThread::sleep(3);
                attempts++;
                continue;
            }
            else
            {
                qCritical() << "Unrecoverable API Error while polling HTTP" << http_code << ":" << QString(responseBuffer);
                return false; // Immediately abort if the URL is wrong (e.g., 404)
            }

            if (obj.contains("error"))
            {
                qCritical() << "API Error while polling:" << obj["error"].toObject()["message"].toString();

#ifdef QT_DEBUG
                LogUtils::safeLog(QString("EventExtractionWorker::waitUntilFileIsActive(...) --- API Error while polling: %1")
                                      .arg(obj["error"].toObject()["message"].toString()),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                return false;
            }
        }

        attempts++;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionWorker::waitUntilFileIsActive(...) --- Waiting for Google to process video... attempt %1 / 60")
                              .arg(attempts),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        QThread::sleep(2);
    }
#endif

    return false;
}
