
#include "AudioTranscriptionWorker.h"

#include "Defs.h"
#include "ExtractorUtils.h"

#include "LogUtils.h"

#include "Url.h"
using namespace utils;

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QProcessEnvironment>

// Deepgram Key = 4e6866bcfcb9ed520189b68350e9a708295e1093
// Name: AI-AudioTranscription
// Tag: env-AINEMo

//Q_GLOBAL_STATIC_WITH_ARGS(const QString, DeepgramKey_Str, ("Token 3c2d450f4fde7898ba831f683558c1ee590e61c7"));

#ifndef USE_LIBCURL
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#else
#include <QThread>

extern "C" {
    #include <curl/curl.h>
}

//static std::once_flag curlInitFlag;

// --- 1. Thread-Safe Global Initialization ---
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

struct CurlListDeleter {
    void operator()(struct curl_slist* slist) const {
        if (slist) curl_slist_free_all(slist);
    }
};

using CurlPtr = std::unique_ptr<CURL, CurlDeleter>;
using CurlListPtr = std::unique_ptr<struct curl_slist, CurlListDeleter>;

size_t AudioTranscriptionWorker::write_TranscribeAudioResponse_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    QFile *file = static_cast<QFile*>(stream);

    if (!file || !file->isOpen()) return 0;

    return file->write(static_cast<char*>(ptr), size * nmemb);
}

int AudioTranscriptionWorker::progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    // ultotal/ulnow are for uploads (Speech2Text)
    // dltotal/dlnow are for downloads (Text2Speech)

    AudioTranscriptionWorker* instance = static_cast<AudioTranscriptionWorker*>(clientp);

    if (!instance) return 0;

    // Check if the thread we are running in has been requested to stop
    if (QThread::currentThread()->isInterruptionRequested())
    {
        return 1; // Returning non-zero tells libcurl to ABORT immediately
    }

    double progress = 0.0;
    if (ultotal > 0)
    {
        progress = (double)ulnow / (double)ultotal * 100.0;

        // You can emit a signal here or log it

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Upload Progress: %1%").arg(progress, 0, 'f', 2),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

    } else if (dltotal > 0)
    {
        double progress = (double)dlnow / (double)dltotal * 100.0;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Download Progress: %1%").arg(progress, 0, 'f', 2),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    if (progress > 0)
    {
        // Using invokeMethod guarantees thread safety if signals are connected across threads
        QMetaObject::invokeMethod(instance, "progressUpdated", Qt::QueuedConnection, Q_ARG(double, progress));
    }

    return 0; // Return non-zero to abort the transfer
}
#endif

AudioTranscriptionWorker::AudioTranscriptionWorker(QObject *parent) :
    QObject(parent),
    m_s2tModel(Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2),
    m_t2sModel(Deepgram_AIS2TT2SModels_t::DeepgramT2SAura2ThaliaEN),
    m_modelAPIKey(AIModels_t::None)
{
    this->m_annotationFile.clear();
    this->m_outputDir.clear();

#ifndef USE_LIBCURL
    // Initialize QNAM
    this->m_manager = new QNetworkAccessManager(this);
#else
//    // Initialize libcurl
//    std::call_once(curlInitFlag, []() {
//        curl_global_init(CURL_GLOBAL_ALL);
//        // Register the cleanup to happen automatically when main() exits
//        std::atexit(curl_global_cleanup);
//    });

    curlManager();
#endif
}

AudioTranscriptionWorker::~AudioTranscriptionWorker()
{
    // TODO
}

bool AudioTranscriptionWorker::operator ==(const AudioTranscriptionWorker &right) const
{
    if ( (this->m_s2tModel != right.m_s2tModel) ||
         (this->m_t2sModel != right.m_t2sModel) ||
         (this->m_annotationFile != right.m_annotationFile) ||
         (this->m_outputDir != right.m_outputDir) ||
         (this->m_modelAPIKey != right.m_modelAPIKey) )
    {
        return false;
    }

    return true;
}

bool AudioTranscriptionWorker::operator !=(const AudioTranscriptionWorker &right) const
{
    return !(*this == right);
}

bool AudioTranscriptionWorker::operator==(const AudioTranscriptionWorker *ptr) const
{
    return (this == ptr);
}

bool AudioTranscriptionWorker::operator!=(const AudioTranscriptionWorker *ptr) const
{
    return (this != ptr);
}

Deepgram_AIS2TT2SModels_t AudioTranscriptionWorker::speech2TextAIModel() const
{
    return this->m_s2tModel;
}

void AudioTranscriptionWorker::setSpeech2TextAIModel(const Deepgram_AIS2TT2SModels_t newS2TAIModel)
{
    if (newS2TAIModel == this->m_s2tModel) return;

    this->m_s2tModel = newS2TAIModel;
}

Deepgram_AIS2TT2SModels_t AudioTranscriptionWorker::text2SpeechAIModel() const
{
    return this->m_t2sModel;
}

void AudioTranscriptionWorker::setText2SpeechAIModel(const Deepgram_AIS2TT2SModels_t newT2SAIModel)
{
    if (newT2SAIModel == this->m_t2sModel) return;

    this->m_t2sModel = newT2SAIModel;
}

AIModels_t AudioTranscriptionWorker::modelAPIKey() const
{
    return this->m_modelAPIKey;
}

void AudioTranscriptionWorker::setModelAPIKey(AIModels_t newModelAPIKey)
{
    if (this->m_modelAPIKey == newModelAPIKey) return;

    this->m_modelAPIKey = newModelAPIKey;
}

const QString AudioTranscriptionWorker::outputDir() const
{
    return this->m_outputDir;
}

void AudioTranscriptionWorker::setOutputDir(const QString &newOutputDir)
{
    if (this->m_outputDir == newOutputDir) return;

    this->m_outputDir = newOutputDir;
}

QString AudioTranscriptionWorker::annotationResultFile() const
{
    return this->m_annotationFile;
}

void AudioTranscriptionWorker::processSpeech2Text(const Url audioFilePath)
{
    QString errMsg;

    if ( ! audioFilePath.isValid() )
    {
        errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(...) --- Error: Audio file path is not valid!");

        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    QFileInfo inFileInfo(audioFilePath.path());

    if ( !inFileInfo.exists() || !inFileInfo.isFile() )
    {
        errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(...) --- Error: No audio file: %1!").arg(audioFilePath.path());

        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    // 1. Prepare File Data
    QFile audioFile(audioFilePath.path());

    if ( ! audioFile.open(QIODevice::ReadOnly))
    {
        errMsg = QString("Could not open audio file for reading.");

        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    QByteArray fileData = audioFile.readAll();
    audioFile.close();

    // 2. Setup output file
    QString ext = inFileInfo.suffix();

    QString filename;

    if ( ! ext.isEmpty() )
    {
        filename = inFileInfo.fileName().replace(ext, "json", Qt::CaseInsensitive);
    } else {
        filename = inFileInfo.fileName() + "json";
    }

    if ( ! this->outputDir().isEmpty() )
    {
        this->m_annotationFile = this->outputDir() + QDir::separator() + filename;
    } else {
        this->m_annotationFile = inFileInfo.absolutePath() + QDir::separator() + filename;
        this->setOutputDir(inFileInfo.absolutePath());
    }

    // 3. Get model Url
    QString urlStr = this->getModelUrl(m_s2tModel);

    if (urlStr.isEmpty())
    {
        errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(const QString &) - Invalid Model URL selection!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit errorOccurred("Invalid Model URL selection.");

        // emit finished();

        return;
    }

    // 4. Get API key
    QString apiKey = ExtractorUtils::getAPIKey(AIModels_t::Deepgram_API);

    if (apiKey.isEmpty())
    {
        errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(const QString &) - API Key is missing.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    // 5. For Content-Type (Auto-detect)
    QMimeDatabase db;

#ifdef USE_LIBCURL
    // 6. Initialize Curl (RAII)
    CurlPtr curl(curl_easy_init());

    if ( !curl )
    {
        errMsg = QString("Failed to initialize LibCurl.");

        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    // 7. Prepare Headers (RAII)
    struct curl_slist *headersRaw = nullptr;

    QString authHeader = "Authorization: Token " + apiKey;
    headersRaw = curl_slist_append(headersRaw, authHeader.toUtf8().constData());

    // Content-Type (Auto-detect)
    QMimeType mime = db.mimeTypeForFile(inFileInfo);
    QString contentType = "Content-Type: " + mime.name();
    // Or force audio/wav if required by your specific flow, but detection is better
    headersRaw = curl_slist_append(headersRaw, contentType.toUtf8().constData());

    // Wrap headers in RAII to ensure they are freed
    CurlListPtr headers(headersRaw);

    // 4. Configure Curl
    curl_easy_setopt(curl.get(), CURLOPT_URL, urlStr.toUtf8().constData());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, fileData.constData());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t)fileData.size());

    // Mobile Network Tolerance
    // 1. Force IPv4: Bypasses buggy IPv6 CGNAT routing common on mobile carriers
    // curl_easy_setopt(curl.get(), CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);

    // 2. Force HTTP/1.1: HTTP/2 frame multiplexing often stalls on unstable 5G links
    // curl_easy_setopt(curl.get(), CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);

    // 3. Fail Fast on Stalls: If speed drops below 50 bytes/sec for 10 seconds,
    // kill the socket immediately and trigger the retry loop. Do not wait for a 408.
    // curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_LIMIT, 50L);
    // curl_easy_setopt(curl.get(), CURLOPT_LOW_SPEED_TIME, 10L);

    // 4. Force Fresh Sockets: Never reuse a connection that previously stalled
    // curl_easy_setopt(curl.get(), CURLOPT_FORBID_REUSE, 1L);

    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 60L); // Allow 60s just to find the tower
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 0L);         // Infinite total time (rely on callbacks)
    curl_easy_setopt(curl.get(), CURLOPT_TCP_KEEPALIVE, 1L);   // Prevent carrier NATs from dropping us
    curl_easy_setopt(curl.get(), CURLOPT_TCP_KEEPIDLE, 10L);   // Ping every 10s of silence
    curl_easy_setopt(curl.get(), CURLOPT_TCP_KEEPINTVL, 10L);

    // Enable the progress meter
    curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);

    // Set the callback function
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, progress_callback);

    // Pass your shared data or 'this' as the 'p' argument in the callback
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, this);

    // Setup Output File
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_TranscribeAudioResponse_callback);

    qint32 maxRetries = 5;
    bool success = false;

    for (qint32 attempt = 1; attempt <= maxRetries; ++attempt)
    {
        QFile outFile(this->m_annotationFile);

        if ( ! outFile.open(QIODevice::WriteOnly | QIODevice::Truncate) )
        {
            errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(const QString &) - Could not create output transcription file!");

#ifdef QT_DEBUG
            LogUtils::safeLog(errMsg,
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            emit errorOccurred(errMsg);

            // TODO Change to send only errorOccurred(...). finished() must be only when AudioTranscriptionWorker finishes the process.
            // emit finished();

            return;
        }

        curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &outFile);

        // 6. Perform curl post
        CURLcode res = curl_easy_perform(curl.get());

        // Flush file
        outFile.close();

        if (res == CURLE_ABORTED_BY_CALLBACK)
        {
            errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(...) --- Transcription cancelled by user.");

            // User clicked Stop
            // We do NOT emit errorOccurred here, so the UI doesn't show a failure popup.
            // We just exit the function, which triggers 'finished' below.
#ifdef QT_DEBUG
            LogUtils::safeLog(errMsg,
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }
        else if (res != CURLE_OK)
        {
            errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(...) --- Network Error: %1").arg(curl_easy_strerror(res));

#ifdef QT_DEBUG
            LogUtils::safeLog(errMsg,
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            if (attempt == maxRetries) emit errorOccurred(errMsg);

        }
        else {
            // Check HTTP response code (e.g., 401 Unauthorized, 404 Not Found)

            long response_code;
            curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);

            if (response_code >= 400)
            {
                errMsg = QString("AudioTranscriptionWorker::processSpeech2Text(...) --- Deepgram API Error (Attempt %1/%2): %3")
                             .arg(attempt)
                             .arg(maxRetries)
                             .arg(response_code);

#ifdef QT_DEBUG
                LogUtils::safeLog(errMsg,
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif

                // Retry ONLY on Timeout (408), Too Many Requests (429), or Server Errors (5xx)
                if (response_code == 408 || response_code == 429 || response_code >= 500)
                {
                    if (attempt == maxRetries) emit errorOccurred(errMsg);
                }
                else
                {
                    // Fatal client error (e.g. 400 Bad Request, 401 Unauthorized)
                    emit errorOccurred(errMsg);
                    break;
                }
            }
            else
            {
                success = true;
                break; // Success! Exit the retry loop.
            }
        }

        // Wait before the next attempt using an exponential backoff
        if (!success && attempt < maxRetries)
        {
            // Bitwise shift calculates: 2s, 4s, 8s, 16s.
            // Gives the 5G modem time to switch towers or recover from a dead zone.
            qint32 sleepMs = (1 << attempt) * 1000;

#ifdef QT_DEBUG
            LogUtils::safeLog(QString("AudioTranscriptionWorker::processSpeech2Text(...) - Sleeping %1 ms before next attempt...")
                                  .arg(sleepMs),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            QThread::msleep(sleepMs); // e.g., wait 2s, then 4s...
        }
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioTranscriptionWorker::processSpeech2Text(const QString &) - Exiting!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit finished();

#else
    QNetworkRequest req((Url(urlStr)));
    req.setRawHeader("Authorization", ("Token " + apiKey).toUtf8());

    req.setHeader(QNetworkRequest::ContentTypeHeader, db.mimeTypeForFileNameAndData(audioFilePath.path(), fileData).name());

    QNetworkReply *reply = this->m_manager->post(req, fileData);

    connect(reply, &QNetworkReply::uploadProgress, this, [this](qint64 sent, qint64 total){
        if(total > 0) emit progressUpdated((double)sent/total*100.0);
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        handleS2TResponse(reply, this->m_lastTranscriptionFile);
    });
#endif
}

void AudioTranscriptionWorker::processText2Speech(const QString &text)
{
    QString errMsg;

    if (text.isEmpty())
    {
        errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - Text for TTS is empty.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit errorOccurred(errMsg);

        emit finished();
    }

    QString urlStr = this->getModelUrl(m_t2sModel);

    if (urlStr.isEmpty())
    {
        errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - Invalid Model URL selection!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit errorOccurred(errMsg);

        emit finished();

        return;
    }

    QString apiKey = ExtractorUtils::getAPIKey(AIModels_t::Deepgram_API);

    if (apiKey.isEmpty())
    {
        errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - API Key is missing.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    // 1. Prepare JSON Payload
    QJsonObject json;
    json["text"] = text;
    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

    // 4. Output File
    QString fileName = QString("tts_output_%1.mp3").arg(QDateTime::currentMSecsSinceEpoch());

    // Use Shared Ptr to keep file alive until lambda finishes
    std::shared_ptr<QFile> outFile = std::make_shared<QFile>(fileName);

    if ( !outFile->open(QIODevice::WriteOnly))
    {
        errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - Could not create output MP3 file.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit errorOccurred(errMsg);
        // emit finished();

        return;
    }

#ifdef USE_LIBCURL
    CurlPtr curl(curl_easy_init());
    if ( !curl )
    {
        errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - Failed to initialize LibCurl.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit errorOccurred(errMsg);

        // emit finished();

        return;
    }

    // 2. Prepare Headers
    struct curl_slist *headersRaw = nullptr;
    QString authHeader = "Authorization: Token " + apiKey;
    headersRaw = curl_slist_append(headersRaw, authHeader.toUtf8().constData());
    headersRaw = curl_slist_append(headersRaw, "Content-Type: application/json");

    CurlListPtr headers(headersRaw);

    curl_easy_setopt(curl.get(), CURLOPT_URL, urlStr.toUtf8().constData());
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, jsonData.constData());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, (long)jsonData.size());

    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, write_TranscribeAudioResponse_callback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &*outFile);

    // Enable the progress meter
    curl_easy_setopt(curl.get(), CURLOPT_NOPROGRESS, 0L);

    // Set the callback function
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFOFUNCTION, progress_callback);

    // Pass your shared data or 'this' as the 'p' argument in the callback
    curl_easy_setopt(curl.get(), CURLOPT_XFERINFODATA, this);

    // Perform curl post
    CURLcode res = curl_easy_perform(curl.get());

    if (res == CURLE_ABORTED_BY_CALLBACK)
    {
        // User clicked Stop - Silent exit
#ifdef QT_DEBUG
        qDebug() << "AudioTranscriptionWorker::processText2Speech(...) --- Text2Speech cancelled by user.";
#endif
    }
    else if (res != CURLE_OK)
    {
        // Network Error
        QString errorMsg = QString("AudioTranscriptionWorker::processText2Speech(...) --- Network Error: %1").arg(curl_easy_strerror(res));

        emit errorOccurred(errorMsg);
    }
    else
    {
        // Success - Check HTTP Code
        long response_code;
        curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &response_code);

        if (response_code >= 400)
        {
            errMsg = QString("AudioTranscriptionWorker::processText2Speech(...) --- Deepgram API Error: %1").arg(response_code);

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
            emit errorOccurred(errMsg);
        }
    }

#ifdef QT_DEBUG
    errMsg = QString("AudioTranscriptionWorker::processText2Speech(const QString &) - Exiting!");
    LogUtils::safeLog(errMsg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit finished();
#else

    QNetworkRequest req((Url(urlStr)));
    req.setRawHeader("Authorization", ("Token " + apiKey).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_manager->post(req, jsonData);

    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 recv, qint64 total){
        if(total > 0) emit progressUpdated((double)recv/total*100.0);
    });

    connect(reply, &QNetworkReply::readyRead, this, [reply, outFile](){
        if(outFile->isOpen()) outFile->write(reply->readAll());
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, outFile](){
        handleT2SResponse(reply, outFile);
    });

#endif
}

#ifndef USE_LIBCURL
void TranscribeAudio::handleS2TResponse(QNetworkReply *reply, const QString &destPath)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit errorOccurred("Network Error: " + reply->errorString());
    } else {
        QFile f(destPath);
        if ( f.open(QIODevice::WriteOnly) )
        {
            f.write(reply->readAll());
        }
        else {
            emit errorOccurred("Write Error");
        }
    }

    emit finished();
}

void TranscribeAudio::handleT2SResponse(QNetworkReply *reply, std::shared_ptr<QFile> destFile)
{
    reply->deleteLater();

    if (destFile->isOpen())
        destFile->close();

    if (reply->error() != QNetworkReply::NoError)
    {
        emit errorOccurred("Network Error: " + reply->errorString());

        destFile->remove();
    }

    emit finished();
}
#endif

QString AudioTranscriptionWorker::getModelUrl(Deepgram_AIS2TT2SModels_t model) const
{
#pragma clang diagnostic ignored "-Wswitch"
    switch (model)
    {
        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova:
        {
            return Constants::Q_QGS_DeepgramS2TNova_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNovaGeneral:
        {
            return Constants::Q_QGS_DeepgramS2TNovaGeneral_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2:
        {
            return Constants::Q_QGS_DeepgramS2TNova2_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralPTBR:
        {
            return Constants::Q_QGS_DeepgramS2TNova2GeneralPTBR_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova2GeneralEN:
        {
            return Constants::Q_QGS_DeepgramS2TNova2GeneralEN_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova3:
        {
            return Constants::Q_QGS_DeepgramS2TNova3_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TNova3General:
        {
            return Constants::Q_QGS_DeepgramS2TNova3General_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TEnhanced:
        {
            return Constants::Q_QGS_DeepgramS2TEnhanced_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TEnhancedGeneral:
        {
            return Constants::Q_QGS_DeepgramS2TEnhancedGeneral_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TBase:
        {
            return Constants::Q_QGS_DeepgramS2TBase_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TBaseGeneral:
        {
            return Constants::Q_QGS_DeepgramS2TBaseGeneral_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperTiny:
        {
            return Constants::Q_QGS_DeepgramS2TWhisperTiny_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperBase:
        {
            return Constants::Q_QGS_DeepgramS2TWhisperBase_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperSmall:
        {
            return Constants::Q_QGS_DeepgramS2TWhisperSmall_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperMedium:
        {
            return Constants::Q_QGS_DeepgramS2TWhisperMedium_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisper:
        {
            return Constants::Q_QGS_DeepgramS2TWhisper_Url->toString();
        }

        case Deepgram_AIS2TT2SModels_t::DeepgramS2TWhisperLarge:
        {
            return Constants::Q_QGS_DeepgramS2TWhisperLarge_Url->toString();
        }
    }

    return QString();
}
