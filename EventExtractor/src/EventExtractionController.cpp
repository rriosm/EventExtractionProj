
#include <QFileInfo>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTextStream>
#include <QTimer>

#include "inc/LogUtils.h"
#include "inc/EventExtractionController.h"
#include "inc/KeywordSearchEngine.h"
#include "inc/PromptManager.h"

// public

EventExtractionController::EventExtractionController(PipelineWorker *parent) :
    PipelineWorker(parent),
    m_eventExtractionWorkerThread(nullptr),
    m_eventExtractionWorker(nullptr),
    m_minConfidenceThreshold(*Constants::Q_QGS_CONFIDENCE_THRESHOLD),
    m_totalChunks(0),
    m_currentChunk(0),
    m_masterFile(nullptr),
    m_taskTemplateType(TaskTemplate::None),
    m_currentIteration(1),
    m_totalIterations(1)
{
    this->m_status.clear();
}

EventExtractionController::~EventExtractionController()
{
    if (this->m_eventExtractionWorkerThread && this->m_eventExtractionWorkerThread->isRunning())
    {
        this->m_eventExtractionWorkerThread->requestInterruption();
        this->m_eventExtractionWorkerThread->quit();

        if ( ! this->m_eventExtractionWorkerThread->wait(3000))
        {
            this->m_eventExtractionWorkerThread->terminate();
            this->m_eventExtractionWorkerThread->wait();
        }
    }

    if (this->m_eventExtractionWorker)
    {
        delete this->m_eventExtractionWorker;
        this->m_eventExtractionWorker = nullptr;
    }

    if (this->m_eventExtractionWorkerThread)
    {
        delete this->m_eventExtractionWorkerThread;
        this->m_eventExtractionWorkerThread = nullptr;
    }
}

bool EventExtractionController::operator==(const EventExtractionController &right) const
{
    if ((this->m_status != right.m_status) ||
        (this->m_model != right.m_model) ||
        (this->m_customModelString != right.m_customModelString) ||
        (this->m_customAPIUrl != right.m_customAPIUrl) ||
        (this->m_customAPIKey != right.m_customAPIKey) ||
        (this->m_videoQueue != right.m_videoQueue) ||
        (this->m_filteredSegmentsWithPromptsList != right.m_filteredSegmentsWithPromptsList) ||
        (this->m_systemPromptCache != right.m_systemPromptCache) ||
        (this->m_userPromptCache != right.m_userPromptCache) ||
        (this->m_totalChunks != right.m_totalChunks) ||
        (this->m_currentChunk != right.m_currentChunk) ||
        (this->m_outputFilePath != right.m_outputFilePath) ||
        (this->m_masterFile != right.m_masterFile) ||
        (this->m_taskTemplateType != right.m_taskTemplateType) ||
        (this->m_currentIteration != right.m_currentIteration) ||
        (this->m_totalIterations != right.m_totalIterations) )
    {
        return false;
    }

    return true;
}

bool EventExtractionController::operator!=(const EventExtractionController &right) const
{
    return !(*this == right);
}

bool EventExtractionController::operator==(const EventExtractionController *ptr) const
{
    return (this == ptr);
}

bool EventExtractionController::operator!=(const EventExtractionController *ptr) const
{
    return (this != ptr);
}

void EventExtractionController::setupModel(AIModels_t newModel,
                                           const QString &newCustomModelString,
                                           const QString &newCustomAPIUrl,
                                           const QString &newCustomAPIKey)
{
    if (newModel != this->m_model)
        this->m_model = newModel;

    if (newCustomModelString != this->m_customModelString)
        this->m_customModelString = newCustomModelString;

    if (newCustomAPIUrl != this->m_customAPIUrl)
        this->m_customAPIUrl = newCustomAPIUrl;

    if (newCustomAPIKey != this->m_customAPIKey)
        this->m_customAPIKey = newCustomAPIKey;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::setupModel(...) - Model, custom model string, API Url and Key."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void EventExtractionController::setOutputFile(const QString &newFilePath)
{
    if (newFilePath.isEmpty() ||
        newFilePath == this->m_outputFilePath) return;

    this->m_outputFilePath = newFilePath;

    // Pass to worker only IF the worker already exists
    if (this->m_eventExtractionWorker)
    {
        this->m_eventExtractionWorker->setOutputFile(newFilePath);
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::setOutputFile(...) - Output file set."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void EventExtractionController::setSystemPrompt(const QString &newSysPrompt)
{
    if (newSysPrompt == this->m_systemPromptCache) return;

    this->m_systemPromptCache = newSysPrompt;
}

TaskTemplate EventExtractionController::taskTemplateType() const
{
    return this->m_taskTemplateType;
}

void EventExtractionController::setTaskTemplateType(TaskTemplate newTaskTemplateType)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::setTaskTemplateType(...) - Output file set."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (newTaskTemplateType == this->m_taskTemplateType) return;

    this->m_taskTemplateType = newTaskTemplateType;
}

qint32 EventExtractionController::totalIterations() const
{
    return this->m_totalIterations;
}

void EventExtractionController::setTotalIterations(qint32 newTotalIterations)
{
    if (newTotalIterations > 0 && newTotalIterations != this->m_totalIterations)
        this->m_totalIterations = newTotalIterations;
}

qint32 EventExtractionController::currentIteration() const
{
    return this->m_currentIteration;
}

void EventExtractionController::setCurrentIteration(qint32 newCurrentIteration)
{
    if (newCurrentIteration > 0 && newCurrentIteration != this->m_currentIteration)
        this->m_currentIteration = newCurrentIteration;
}

void EventExtractionController::addUserMessage(const QString &content)
{
    if (content.isEmpty()) return;

    if (!this->m_eventExtractionWorker) return;

    this->m_eventExtractionWorker->addUserMessage(content);
}

void EventExtractionController::addUserMessageWithImage(const QString &text, const QString &imagePath)
{
    if (imagePath.isEmpty()) return;

    if (!this->m_eventExtractionWorker) return;

    this->m_eventExtractionWorker->addUserMessageWithImage(text, imagePath);
}

void EventExtractionController::addUserMessageWithVideo(const QString &text, const QString &videoPath)
{
    if (videoPath.isEmpty()) return;

    if (!this->m_eventExtractionWorker) return;

    this->m_eventExtractionWorker->addUserMessageWithVideo(text, videoPath);
}

void EventExtractionController::addUserMessageWithVideosList(const QString &text, const QStringList &videosPaths)
{
    this->m_userPromptCache = text;
    this->m_videoQueue = videosPaths;
    this->m_totalChunks = videosPaths.size();
    this->m_currentChunk = 0;
}

void EventExtractionController::addUserMessageFilteredSegsWithPrompts(const SegmentsWithPrompts & filteredSegsWithPrompts)
{
    if (filteredSegsWithPrompts == this->m_filteredSegmentsWithPromptsList) return;

    this->m_filteredSegmentsWithPromptsList = filteredSegsWithPrompts;

    this->m_totalChunks = this->m_filteredSegmentsWithPromptsList.size();
    this->m_currentChunk = 0;
}

void EventExtractionController::addMessageHistory(const MessageHistory &newMsgHistory)
{
    if (!this->m_eventExtractionWorker) return;

    this->m_eventExtractionWorker->addMessageHistory(newMsgHistory);
}

void EventExtractionController::addTermForSearching(const QString &term)
{
    if (term.isEmpty()) return;

    this->m_searchTerms.append(term);
}

void EventExtractionController::addTermsForSearching(const QStringList &searchTerms)
{
    if (searchTerms.isEmpty()) return;

    this->m_searchTerms.append(searchTerms);
}

const QStringList EventExtractionController::searchTerms() const
{
    return this->m_searchTerms;
}

void EventExtractionController::setSearchType(KeywordMatchType_t sType)
{
    this->m_searchTermsType = sType;
}

KeywordMatchType_t EventExtractionController::searchType() const
{
    return this->m_searchTermsType;
}

void EventExtractionController::clearSearchTerms()
{
    if (this->m_searchTerms.isEmpty()) return;

    this->m_searchTerms.clear();
}

double EventExtractionController::minConfidenceThreshold() const
{
    return this->m_minConfidenceThreshold;
}

void EventExtractionController::setMinConfidenceThreshold(double newMinConfidenceThreshold)
{
    if (newMinConfidenceThreshold < 0.00 || newMinConfidenceThreshold == this->m_minConfidenceThreshold) return;

    this->m_minConfidenceThreshold = newMinConfidenceThreshold;
}

// public slots
void EventExtractionController::cancelEventExtraction()
{
    if (this->m_eventExtractionWorkerThread && this->m_eventExtractionWorkerThread->isRunning())
    {
        QString msg("Aborting and cleaning up...");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::cancelEventExtraction() --- %1")
                          .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        this->setStatus(msg);

        emit statusChanged(msg);
        // emit stopRequested();
        emit requestAbort();

        this->m_eventExtractionWorkerThread->requestInterruption();
    }
}

void EventExtractionController::handleInput(FileInfo fInfo)
{
    QString msg;

    if (fInfo.url.isEmpty() || ! fInfo.url.isValid())
    {
        msg = QString("[EVENT CONTROLLER] Manifest file is empty/invalide...");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- %1")
                              .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(msg);

        return;
    }

    VideoSegmentInfo::VideoSegmentInfoList_t segmentList = this->parseSynchronizedManifest(fInfo);

    if (segmentList.isEmpty())
    {
        msg = QString("Parse error: segments list is empty");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- %1")
                              .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(msg);

        return;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- Ready to process %1 segments in memory!")
                          .arg(segmentList.size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);

    LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- Search terms current size: %1")
                          .arg(this->searchTerms().size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);

    LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- Search terms content: %1").arg(this->searchTerms().join(", ")),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    VideoSegmentInfo::VideoSegmentInfoList_t filteredSegments;

    // If there is no search terms, submit all video segments.
    if (this->searchTerms().isEmpty())
    {
        filteredSegments = segmentList;

        msg = QString("Search terms list is empty");
    }
    else
    {
        // Filter transcripts that have events.
        filteredSegments = KeywordSearchEngine::filterSegments(segmentList,
                                                               this->searchTerms(),
                                                               this->searchType(),
                                                               this->minConfidenceThreshold());

        msg = QString("Search terms list not empty. Segments filtered!");
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- %1")
                          .arg(msg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(msg);

    // Output the results

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- [EVENT CONTROLLER] Total (filtered) segments found: %1!")
                          .arg(filteredSegments.size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (filteredSegments.size() > 0)
    {
        for (const auto& seg : qAsConst(filteredSegments))
        {
            qDebug() << "Matched Segment ID:" << seg.id().toHex(); // Assuming toHex() outputs the string format
        }

        // A list of videos paths and its user prompts
        SegmentsWithPrompts segmentsWithPrompts = this->prepareFilteredSegsWithPrompts(filteredSegments);

#ifdef QT_DEBUG
        for (auto it = segmentsWithPrompts.data.constBegin(); it != segmentsWithPrompts.data.constEnd(); ++it)
        {
            LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- [EVENT CONTROLLER] Id: %1!")
                                  .arg(segmentsWithPrompts.contextFor(it.key())->segmentId),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
        }
#endif

        this->addUserMessageFilteredSegsWithPrompts(segmentsWithPrompts);

        msg = QString("Events extraction ready to start!");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- [EVENT CONTROLLER] Filtered segments size: %1!")
                              .arg(filteredSegments.size()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(msg);

        // Start events extraction
        QTimer::singleShot(10, this, &EventExtractionController::startEventExtraction);
    }
    else
    {
        msg = QString("No segments matched the provided search terms.");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::handleInput(... ) --- %1!")
                              .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(msg);
    }
}

void EventExtractionController::onChunkFinished(bool success)
{
    if (!success)
    {
        QString errMsg = QString("Chunk failed. Attempting to continue to the next segment to salvage the rest of the match...");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::onChunkFinished() --- [EVENT CONTROLLER] %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // Use a singleShot timer to decouple the call stack and give the event loop a chance to breathe.
    // Also gives the Google API a 3-second cooldown before we hammer it with the next video.
    QTimer::singleShot(10000, this, &EventExtractionController::processNextChunk);
}

void EventExtractionController::onChunkResponseReceived(const QString &fullSegmentHexId, const QString &response)
{
    // Write the AI's response directly into our master document
    if (this->m_masterFile && this->m_masterFile->isOpen())
    {
        QTextStream out(this->m_masterFile);
        out.setCodec("UTF-8");

        // 1. Create a readable Git-style short ID (first 8 characters)
        // Fallback safely just in case a malformed ID is passed
        QString shortId = fullSegmentHexId.length() > 8 ? fullSegmentHexId.left(8) : fullSegmentHexId;

        // 2. Write the Tag
        out << QString(*Constants::Q_QGS_CLIP_ITERATON_TAG_PATTERN)
                   .arg(shortId)
                   .arg(this->currentIteration());

        out << response << "\n\n";
        out.flush();

#ifdef QT_DEBUG

        LogUtils::safeLog(QString("EventExtractionController::onChunkResponseReceived() --- [EVENT CONTROLLER] " + *Constants::Q_QGS_CLIP_ITERATON_TAG_PATTERN)
                          .arg(shortId)
                          .arg(this->currentIteration()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);

        LogUtils::safeLog(QString("EventExtractionController::onChunkResponseReceived() --- [EVENT CONTROLLER] Chunk response successfully appended to master file."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

void EventExtractionController::processNextChunkNoMapList()
{
    if (this->m_videoQueue.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::processNextChunk() --- [EVENT CONTROLLER] All video chunks processed successfully!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        // Close Master File
        if (this->m_masterFile && this->m_masterFile->isOpen())
        {
            this->m_masterFile->close();
            delete this->m_masterFile;
            this->m_masterFile = nullptr;
        }

        // Shut down the worker thread gracefully. Synchronous Inner Teardown
        if (this->m_eventExtractionWorkerThread)
        {
            if (this->m_eventExtractionWorkerThread->isRunning())
            {
                this->m_eventExtractionWorkerThread->quit();
                this->m_eventExtractionWorkerThread->wait();
            }

            // Safely delete inner components now that the thread is physically dead
            if (this->m_eventExtractionWorker)
            {
                delete this->m_eventExtractionWorker;
                this->m_eventExtractionWorker = nullptr;
            }

            delete this->m_eventExtractionWorkerThread;
            this->m_eventExtractionWorkerThread = nullptr;
        }

        emit statusChanged("Event extraction Complete.");

        // Tells main.cpp to shut down the app
        FileInfo fInfo(this->m_outputFilePath, FileType_t::EventExtractionManifest, true);
        emit finished(fInfo);

        return;
    }

    // --- SEGFAULT SAFETY CHECK ---
    if (!this->m_eventExtractionWorker)
    {
        qCritical() << "[EVENT CONTROLLER] Fatal: Worker object is null. Aborting queue.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::processNextChunk() --- [EVENT CONTROLLER] Fatal: Worker object is null. Aborting queue."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    this->m_currentChunk++;
    QString nextVideoPath = this->m_videoQueue.takeFirst();
    QFileInfo fileInfo(nextVideoPath);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::processNextChunk() ---"
                              "\n======================================================="
                              "\n[EVENT CONTROLLER] Starting Chunk %1 of %2: %3"
                              "\n=======================================================")
                          .arg(this->m_currentChunk)
                          .arg(this->m_totalChunks)
                          .arg(fileInfo.fileName()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Reset the Worker for a fresh run
    this->m_eventExtractionWorker->clearMessages();

    // 2. Inject the cached prompts and ONLY the current video
    this->m_eventExtractionWorker->setSystemPrompt(this->m_systemPromptCache);
    this->m_eventExtractionWorker->addUserMessageWithVideo(this->m_userPromptCache, fileInfo.absoluteFilePath());

    // Disable worker-level file saving (the Controller handles saving now)
    this->m_eventExtractionWorker->setOutputFile("");

    // 3. Fire the pipeline!
    // this->m_eventExtractionWorker->process();
    QTimer::singleShot(30000, this, [this]() {
        QMetaObject::invokeMethod(this->m_eventExtractionWorker, "process", Qt::QueuedConnection);
    });
}

void EventExtractionController::processNextChunk()
{
    if (this->m_filteredSegmentsWithPromptsList.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::processNextChunk() --- [EVENT CONTROLLER] All video chunks processed successfully!"),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        // Close Master File
        if (this->m_masterFile && this->m_masterFile->isOpen())
        {
            this->m_masterFile->close();
            delete this->m_masterFile;
            this->m_masterFile = nullptr;
        }

        // Shut down the worker thread gracefully. Synchronous Inner Teardown
        if (this->m_eventExtractionWorkerThread)
        {
            if (this->m_eventExtractionWorkerThread->isRunning())
            {
                this->m_eventExtractionWorkerThread->quit();
                this->m_eventExtractionWorkerThread->wait();
            }

            // Safely delete inner components now that the thread is physically dead
            if (this->m_eventExtractionWorker)
            {
                delete this->m_eventExtractionWorker;
                this->m_eventExtractionWorker = nullptr;
            }

            delete this->m_eventExtractionWorkerThread;
            this->m_eventExtractionWorkerThread = nullptr;
        }

        emit statusChanged("Event extraction Complete.");

        // Tells main.cpp to shut down the app
        FileInfo fInfo(this->m_outputFilePath, FileType_t::EventExtractionManifest, true);
        emit finished(fInfo);

        return;
    }

    // --- SEGFAULT SAFETY CHECK ---
    if (!this->m_eventExtractionWorker)
    {
        qCritical() << "[EVENT CONTROLLER] Fatal: Worker object is null. Aborting queue.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::processNextChunk() --- [EVENT CONTROLLER] Fatal: Worker object is null. Aborting queue."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        return;
    }

    this->m_currentChunk++;

    QString nextVideoPath = this->m_filteredSegmentsWithPromptsList.firstKey();

    SegmentsWithPrompts::SegmentContext currentContext = this->m_filteredSegmentsWithPromptsList.take(nextVideoPath);
    // QString currentSegmentId = this->m_filteredSegmentsWithPromptsList.segmentIdFor(nextVideoPath);

    QFileInfo fileInfo(nextVideoPath);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::processNextChunk() ---"
                              "\n======================================================="
                              "\n[EVENT CONTROLLER] Starting Chunk %1 of %2: %3 -- Segment Id: %4"
                              "\n=======================================================")
                          .arg(this->m_currentChunk)
                          .arg(this->m_totalChunks)
                          .arg(nextVideoPath,
                               currentContext.segmentId),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Reset the Worker for a fresh run
    this->m_eventExtractionWorker->clearMessages();

    this->m_eventExtractionWorker->setSegmentId(currentContext.segmentId);

    // 3. Inject the cached prompts and ONLY the current video
    this->m_eventExtractionWorker->setSystemPrompt(this->m_systemPromptCache);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::processNextChunk() ---"
                              "\n======================================================="
                              "\n[EVENT CONTROLLER] System Profile: "
                              "\n %1 "
                              "\n=======================================================")
                          .arg(this->m_systemPromptCache),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::processNextChunk() ---"
                              "\n======================================================="
                              "\n[EVENT CONTROLLER] User Prompt: "
                              "\n %1 "
                              "\n=======================================================")
                          .arg(currentContext.userPrompt),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->m_eventExtractionWorker->addUserMessageWithVideo(currentContext.userPrompt, fileInfo.absoluteFilePath());

    // 4. Disable worker-level file saving (the Controller handles saving now)
    this->m_eventExtractionWorker->setOutputFile("");

    // 5. Fire the pipeline!
    // this->m_eventExtractionWorker->process();
    QMetaObject::invokeMethod(this->m_eventExtractionWorker, "process", Qt::QueuedConnection);

    emit chunkFinished(true);
}

void EventExtractionController::setStatus(const QString &newStatus)
{
    if (newStatus != this->m_status)
        this->m_status = newStatus;
}

void EventExtractionController::startEventExtraction()
{
    QString errMsg;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("[EVENT CONTROLLER] Starting Chunked Event Extraction Pipeline...!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->m_eventExtractionWorkerThread && this->m_eventExtractionWorkerThread->isRunning())
    {
        QString errMsg = "An event extraction is already in progress.";

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- %1.")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(errMsg);

        return;
    }

    if (this->m_model == AIModels_t::None)
    {
        errMsg = QString("Model not set!");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- %1.")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);

        return;
    }

    // 1. Prepare Master Output File
    this->m_masterFile = new QFile(this->m_outputFilePath);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- [EVENT CONTROLLER] Output file path: %1")
                          .arg(this->m_outputFilePath),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (!this->m_masterFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        errMsg = QString("Critical Error: Cannot open master file for appending: !").arg(this->m_masterFile->fileName());

        qCritical() << errMsg;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- [EVENT CONTROLLER] %1")
                              .arg(errMsg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        FileInfo fInfo(this->m_outputFilePath, FileType_t::EventExtractionManifest, false);
        emit finished(fInfo);

        return;
    }

    if (!this->m_eventExtractionWorkerThread)
    {
        this->m_eventExtractionWorkerThread = new QThread;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- Event extraction worker thread created."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    if (!this->m_eventExtractionWorker)
    {
        this->m_eventExtractionWorker = new EventExtractionWorker;

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Event extraction worker created."),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    this->m_eventExtractionWorker->setModel(this->m_model, this->m_customModelString, this->m_customAPIUrl, this->m_customAPIKey);
    this->m_eventExtractionWorker->setContentType(ContentMimeType_t::ApplicationJson);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- Model set."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->m_eventExtractionWorker->moveToThread(this->m_eventExtractionWorkerThread);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- EventExtractionWorker moved to QThread."),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QObject::connect(this->m_eventExtractionWorker,
                     &EventExtractionWorker::progressUpdated,
                     this,
                     [this](qint64 bytesSent, qint64 bytesTotal){
                         // Calculate percentage manually
                         double p = 0.0;
                         if (bytesTotal > 0){
                             p = (static_cast<double>(bytesSent) / bytesTotal) * 100.0;
                         }

                         // Emit the Controllers'signal
                         emit progressUpdated(static_cast<qint32>(p));
    });

    QObject::connect(this->m_eventExtractionWorker,
                     &EventExtractionWorker::errorOccurred,
                     this,
                     &EventExtractionController::handleWokerError);

    // Cross-thread progress updates
    QObject::connect(this->m_eventExtractionWorker,
                     &EventExtractionWorker::statusUpdated,
                     this,
                     &EventExtractionController::statusChanged);

    QObject::connect(this->m_eventExtractionWorker,
                     &EventExtractionWorker::responseReceived,
                     this,
                     &EventExtractionController::onChunkResponseReceived);

    QObject::connect(this->m_eventExtractionWorker,
                     &EventExtractionWorker::finished,
                     this,
                     &EventExtractionController::onChunkFinished);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Event extraction worker connections established.\n\n"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->m_eventExtractionWorkerThread->start();

    QMetaObject::invokeMethod(this, "processNextChunk", Qt::QueuedConnection);

    QString statusMsg("Initializing event extraction...");

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::startEventExtraction() --- %1\n\n").arg(statusMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(statusMsg);
}

void EventExtractionController::handleFinished()
{
    QString msg = QString("Event extraction Complete.");

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::handleFinished() --- %1")
                      .arg(msg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(msg);

    // Shut down the inner thread synchronously first
    if (this->m_eventExtractionWorkerThread)
    {
        if (this->m_eventExtractionWorkerThread->isRunning())
        {
            this->m_eventExtractionWorkerThread->quit();
            this->m_eventExtractionWorkerThread->wait();
        }

        if (this->m_eventExtractionWorker)
        {
            delete this->m_eventExtractionWorker;
            this->m_eventExtractionWorker = nullptr;
        }

        delete this->m_eventExtractionWorkerThread;
        this->m_eventExtractionWorkerThread = nullptr;
    }

    FileInfo fInfo(this->m_outputFilePath, FileType_t::EventExtractionManifest, true);
    emit finished(fInfo);
}

void EventExtractionController::handleWokerError(QString errorMsg)
{
    QString errMsg = QString("Event extraction failed: %1").arg(errorMsg);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::handleWorkerError(...) --- %1")
                      .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Notify the UI via the public signal
    emit error(errorMsg);
    emit statusChanged(errMsg);
}

bool EventExtractionController::loadJsonFile(const QString &filePath, QJsonDocument &jsonDoc)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly))
    {
        this->m_lastError = "Cannot open file: " + filePath;
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    jsonDoc = QJsonDocument::fromJson(data, &parseError);

    if (jsonDoc.isNull())
    {
        this->m_lastError = QString("JSON parse error in %1: %2")
        .arg(filePath, parseError.errorString());

        return false;
    }

    return true;
}

VideoSegmentInfo::VideoSegmentInfoList_t EventExtractionController::parseSynchronizedManifest(FileInfo manifestFInfo)
{
    VideoSegmentInfo::VideoSegmentInfoList_t segmentList;
    QJsonDocument manifestDoc;

    if (manifestFInfo.url.isEmpty() || ! manifestFInfo.url.isValid())
    {
        return segmentList;
    }

    // Utilize the existing loadJsonFile method which also handles setting lastError
    if (!this->loadJsonFile(manifestFInfo.url.path(), manifestDoc))
    {
        qWarning() << "Failed to load manifest for parsing:" << m_lastError;
        return segmentList;
    }

    QJsonObject rootObj = manifestDoc.object();

    // Verify the segments array exists
    if (!rootObj.contains(QStringLiteral("segments")) || !rootObj[QStringLiteral("segments")].isArray())
    {
        m_lastError = "Manifest does not contain a valid 'segments' array.";
        qWarning() << m_lastError;
        return segmentList;
    }

    QJsonArray segmentsArray = rootObj[QStringLiteral("segments")].toArray();

    // Iterate through the array and parse each object using VideoSegmentInfo's built-in deserializer
    for (const QJsonValue &value : qAsConst(segmentsArray))
    {
        if (value.isObject())
        {
            QJsonObject segmentObj = value.toObject();

            // 1. Use the built-in deserializer for standard fields
            VideoSegmentInfo segmentInfo = VideoSegmentInfo::fromJson(segmentObj);

            // 2. Extract Audio object to modify it
            Audio audio = segmentInfo.audioTrack();

            // 3. Map annotation "file_path"
            if (segmentObj.contains(QStringLiteral("annotation")))
            {
                QString annotPath = segmentObj[QStringLiteral("annotation")].toObject()[QStringLiteral("file_path")].toString();
                QFile file(annotPath);

                if (file.open(QIODevice::ReadOnly))
                {
                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    audio.setAnnotation(doc.object());
                    file.close();
                }
            }

            // 4. Map vtt_transcript "file_path", "startTimeSec", "endTimeSec"
            if (segmentObj.contains(QStringLiteral("vtt_transcript")))
            {
                QJsonObject vttObject = segmentObj[QStringLiteral("vtt_transcript")].toObject();

                if (vttObject.contains(QStringLiteral("file_path")))
                {
                    QString vttPath = vttObject[QStringLiteral("file_path")].toString();
                    QFile file(vttPath);

                    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
                    {
                        QTextStream in(&file);
                        in.setCodec("UTF-8");
                        audio.setTranscription(in.readAll());
                        file.close();
                    }
                }

                if (vttObject.contains(QStringLiteral("timing")))
                {
                    QJsonObject vttTiming = vttObject[QStringLiteral("timing")].toObject();

                    double vttStartTimeSec = vttTiming.value(QStringLiteral("startTimeSec")).toDouble(0.0);
                    double vttEndTimeSec   = vttTiming.value(QStringLiteral("endTimeSec")).toDouble(0.0);

                    audio.setTranscriptionTimeSec(static_cast<qint32>(qRound(vttStartTimeSec)),
                                                  static_cast<qint32>(qRound(vttEndTimeSec)));
                }
            }

            // 5. Re-assign the modified audio track to the segment
            segmentInfo.setAudioTrack(audio);
            segmentList.append(segmentInfo);
        }
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("EventExtractionController::parseSynchronizedManifest(...) --- Successfully parsed %1 VideoSegmentInfo objects.")
                          .arg(segmentList.size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return segmentList;
}

SegmentsWithPrompts EventExtractionController::prepareFilteredSegsWithPrompts(const VideoSegmentInfo::VideoSegmentInfoList_t & filteredSegments)
{
    QString userPrompt;
    ClipData clipData;

    SegmentsWithPrompts filteredSegmentsWithPrompts;

    const QTime baseTime(0, 0, 0);

    for (const auto &segment : filteredSegments)
    {
        QString currentClipId = segment.id().toHex();
        clipData.add("clip_video_id", currentClipId);

        // Convert double seconds to integer milliseconds (rounded to nearest ms)
        qint32 startMs = qRound(segment.startTime() * 1000.0);
        qint32 endMs   = qRound(segment.endTime() * 1000.0);

        QTime startTime = baseTime.addMSecs(startMs);
        QTime endTime   = baseTime.addMSecs(endMs);

        // If millisecond is to be displayed in the string, use "HH:mm:ss.zzz"
        clipData.add("clip_start_video", startTime.toString("HH:mm:ss.zzz"));
        clipData.add("clip_end_video", endTime.toString("HH:mm:ss.zzz"));
        clipData.add("slowdown_factor", QString::number(segment.slowdownFactor()));

        const Audio &audio = segment.audioTrack();
        if (audio.transcription().isEmpty())
        {
            clipData.add("paste_vtt_here", "No VTT available");
            clipData.add("vtt_start_time", "00:00:00.000");
            clipData.add("vtt_end_time",   "00:00:00.000");

        } else {
            QTime startTime = baseTime.addSecs(audio.transcriptionStartTimeSec());
            QTime endTime   = baseTime.addSecs(audio.transcriptionEndTimeSec());

            clipData.add("vtt_start_time", startTime.toString("HH:mm:ss.zzz"));
            clipData.add("vtt_end_time", endTime.toString("HH:mm:ss.zzz"));

            clipData.add("paste_vtt_here", segment.audioTrack().transcription());
        }

        clipData.add("camera_angle", "Wide / Tactical");
        clipData.add("visibility", "Clear / Floodlights");
        clipData.add("ambiguities", "None");

        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"clip_video_id\"]: " << clipData.valueFor("clip_video_id");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"clip_start_video\"]: " << clipData.valueFor("clip_start_video");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"clip_end_video\"]: " << clipData.valueFor("clip_end_video");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"slowdown_factor\"]: " << clipData.valueFor("slowdown_factor");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"paste_vtt_here\"]: " << clipData.valueFor("paste_vtt_here");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"vtt_start_time\"]: " << clipData.valueFor("vtt_start_time");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"vtt_end_time\"]: " << clipData.valueFor("vtt_end_time");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"camera_angle\"]: " << clipData.valueFor("camera_angle");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"visibility\"]: " << clipData.valueFor("visibility");
        qDebug() << "EventExtractionController::prepareFilteredSegsWithPrompts(...) ------- clipData[\"ambiguities\"]: " << clipData.valueFor("ambiguities");

        userPrompt = PromptManager::instance().getPreparedTask(this->taskTemplateType(), clipData);

        if (this->taskTemplateType() == TaskTemplate::Video_Baseline_Analysis_Task)
        {
            qDebug() << "EventExtractionController() ------- Task template is: Baseline";
        } else if (this->taskTemplateType() == TaskTemplate::Video_Analysis_Task)
        {
            qDebug() << "EventExtractionController() ------- Task template is: system profile";
        }

        filteredSegmentsWithPrompts.add(segment.filePath(), currentClipId, userPrompt);
        clipData.clear();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("\nEventExtractionController::parseSynchronizedManifest(...) --- Filtered Segment: %1 -- %2.")
                              .arg(segment.filePath(),
                                   filteredSegmentsWithPrompts.segmentIdFor(segment.filePath())),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);

        LogUtils::safeLog(QString("EventExtractionController::parseSynchronizedManifest(...) --- User prompt: %1.")
                              .arg(userPrompt),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    return filteredSegmentsWithPrompts;
}

QString EventExtractionController::formatMinutesSeconds(double totalSeconds, qint32 minuteDigits)
{
    qint64 totalSecs = qRound64(totalSeconds);
    qint64 minutes = totalSecs / 60;
    qint64 seconds = totalSecs % 60;

    // %1: padded with zeros to 'minuteDigits' width (e.g., 3 digits -> 000, 005, 125)
    // %2: 2 digits padded with zeros for seconds
    return QString("%1:%2")
        .arg(minutes, minuteDigits, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}
