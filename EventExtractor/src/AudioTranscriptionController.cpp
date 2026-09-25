
#include <QGlobalStatic>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "inc/AudioTranscriptionController.h"

#include "inc/LogUtils.h"

AudioTranscriptionController::AudioTranscriptionController(PipelineWorker *parent)
    : PipelineWorker(parent),
      m_workerThread(nullptr),
      m_worker(nullptr),
      m_model(Deepgram_AIS2TT2SModels_t::NONE),
      m_clipSec(0.00)
{
    this->m_annotationDir.clear();
    this->m_annotationClipsOutputDir.clear();
}

AudioTranscriptionController::~AudioTranscriptionController()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        this->m_workerThread->requestInterruption();    // Tell the curl callback to return 1
        this->m_workerThread->quit();

        if ( ! this->m_workerThread->wait(3000) )       // Wait up to 3 seconds
        {
            this->m_workerThread->terminate();          // Force kill if it hangs
            this->m_workerThread->wait();               // Still wait to ensure cleanup
        }
    }
}

bool AudioTranscriptionController::operator ==(const AudioTranscriptionController & right) const
{
    if ( (this->m_model != right.m_model) ||
         (this->m_annotationDir != right.m_annotationDir) )
    {
        return false;
    }

    return true;
}

bool AudioTranscriptionController::operator !=(const AudioTranscriptionController & right) const
{
    return !(*this == right);
}

bool AudioTranscriptionController::operator==(const AudioTranscriptionController *ptr) const
{
    return (this == ptr);
}

bool AudioTranscriptionController::operator!=(const AudioTranscriptionController *ptr) const
{
    return (this != ptr);
}

Deepgram_AIS2TT2SModels_t AudioTranscriptionController::model() const
{
    return this->m_model;
}

void AudioTranscriptionController::setModel(Deepgram_AIS2TT2SModels_t newModel)
{
    if (newModel == this->m_model) return;

    this->m_model = newModel;
}

Url AudioTranscriptionController::audioAnnotationPath() const
{
    return m_audioAnnotationPath;
}

void AudioTranscriptionController::setAudioAnnotationPath(const Url &newAudioAnnotationPath)
{
    if (newAudioAnnotationPath == this->m_audioAnnotationPath) return;

    this->m_audioAnnotationPath = newAudioAnnotationPath;
}

void AudioTranscriptionController::setAudioAnnotationPath(const QString &newAudioAnnotationPath)
{
    this->setAudioAnnotationPath(Url(newAudioAnnotationPath));
}

qreal AudioTranscriptionController::clipSec() const
{
    return this->m_clipSec;
}

void AudioTranscriptionController::setClipSec(qreal newClipSec)
{
    if (qFuzzyCompare(newClipSec +1.00, this->m_clipSec + 1.00)) return;

    this->m_clipSec = newClipSec;
}

const QString &AudioTranscriptionController::annotationDir() const
{
    return this->m_annotationDir;
}

void AudioTranscriptionController::setAnnotationDir(const QString &newAnnotationDir)
{
    if (this->m_annotationDir == newAnnotationDir) return;

    this->m_annotationDir = newAnnotationDir;
}

QString AudioTranscriptionController::annotationClipsOutputDir() const
{
    return this->m_annotationClipsOutputDir;
}

void AudioTranscriptionController::setAnnotationClipsOutputDir(const QString &newAnnotationClipsOutputDir)
{
    if (newAnnotationClipsOutputDir == this->m_annotationClipsOutputDir) return;

    this->m_annotationClipsOutputDir = newAnnotationClipsOutputDir;
}

void AudioTranscriptionController::cancelTranscription()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        QString errMsg = QString("Stopping transcription...");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioTranscriptionController::cancelTranscription() --- " + errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(errMsg);

        // This sets a flag that QThread::currentThread()->isInterruptionRequested()
        // will return 'true' for inside the worker thread.
        this->m_workerThread->requestInterruption();

        // Note: We do NOT call quit() here. We wait for the curl callback
        // to see the flag, abort the transfer, and let the worker finish naturally.
    }
}

void AudioTranscriptionController::handleInput(FileInfo info)
{
    QString errMsg;

    if (info.url.isEmpty() || ! info.url.isValid())
    {
        errMsg = QString("AudioTranscriptionController::handleInput(...) --- Audio input file is invalid!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);

        return;
    }

    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        errMsg = QString("Transcription is already in progress!");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioTranscriptionController::handleInput(...) --- " + errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(errMsg);

        return;
    }

    if (this->model() == Deepgram_AIS2TT2SModels_t::NONE)
    {
        errMsg = QString("AudioTranscriptionController::handleInput(...) --- Model not supported!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);
        return;
    }

    QFileInfo audioFile(info.url.path());

    if ( ! audioFile.isFile() || ! audioFile.exists() )
    {
        errMsg = QString("AudioTranscriptionController::handleInput(...) --- Audio file is invalid!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);
        return;
    }

    // 1. Initialize Worker and Thread
    this->m_workerThread = new QThread(); // No parent, handled by deleteLater
    this->m_worker = new AudioTranscriptionWorker();

    // 2. Configure the worker
    this->m_worker->setOutputDir(this->annotationDir());
    this->m_worker->setSpeech2TextAIModel(this->model());

    this->m_worker->moveToThread(this->m_workerThread);

    // 3. Setup Signal Forwarding
    // Progress: Map double (0.0-100.0) to int (0-100) for progress bar
    QObject::connect(this->m_worker,
                     &AudioTranscriptionWorker::progressUpdated,
                     this,
                     [this](double p){
        emit progressChanged(static_cast<int>(p));
    }, Qt::QueuedConnection);

    // 4. Error Handling
    // (Assuming you add an errorOccurred signal to TranscribeAudio)
    QObject::connect(this->m_worker,
                     &AudioTranscriptionWorker::errorOccurred,
                     this,
                     &AudioTranscriptionController::handleWorkerError);

    // 5. Lifecycle Management. Safe Start Lambda (Added 'this' context)
    auto worker = this->m_worker;
    Url audioFileUrl = info.url;

    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this->m_worker,
                     [worker, audioFileUrl]() {
            if (worker) {
                worker->processSpeech2Text(audioFileUrl);
            }
        }, Qt::QueuedConnection);

    // 7. Thread Stopped -> UI Notification (Data extraction happens here)
    QObject::connect(this->m_worker,
                     &AudioTranscriptionWorker::finished,
                     this,
                     &AudioTranscriptionController::handleFinished);

    // 8. Auto-cleanup: delete worker when thread stops, delete thread when finished
    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_worker,
                     &QObject::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_workerThread,
                     &QObject::deleteLater);

    // Ensure pointers are zeroed so the "is running" check works next time
    QObject::connect(this->m_workerThread,
                     &QObject::destroyed,
                     this,
                     [this](QObject *obj){
                 if (this->m_workerThread == obj) {
                     this->m_workerThread = nullptr;
                 }
             });

    QObject::connect(this->m_worker,
                     &QObject::destroyed,
                     this,
                     [this](QObject* obj){
        if (this->m_worker == obj) {
            this->m_worker = nullptr;
        }
    });

    // 9. Start!
    this->m_workerThread->start();

    errMsg = QString("Initializing Deepgram AI...");

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioTranscriptionController::handleInput(...) --- " + errMsg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(errMsg);
}

void AudioTranscriptionController::handleFinished()
{
    QString msg = QString("Transcription Complete.");

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioTranscriptionController::handleFinished() --- " + msg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(msg);

    this->setAudioAnnotationPath(this->m_worker->annotationResultFile());

    FileInfo fInfo(this->m_worker->annotationResultFile(), FileType_t::AnnotationFile);

    // Alert annotation file is ready!
    emit finished(fInfo);

    // this->segmentJsonContent(this->m_worker->annotationResultFile(), this->annotationClipsOutputDir(), this->clipSec());

    if (this->m_workerThread) this->m_workerThread->quit();
}

void AudioTranscriptionController::handleWorkerError(QString msg)
{
    QString errMsg = QString("Transcription failed: ").arg(msg);

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioTranscriptionController::handleWorkerError(...) --- " + errMsg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Notify the UI via the public signal
    emit error(msg);
    emit statusChanged(errMsg);

    // 2. Stop the thread
    if (this->m_workerThread) this->m_workerThread->quit();
}

void AudioTranscriptionController::segmentJsonContent(const QString &deepgramJsonPath, const QString &outputDir, qint32 clipSec)
{
    if (deepgramJsonPath.isEmpty() || clipSec <= 0) return;

    // Load and Parse Deepgram JSON
    QFile inputFile(deepgramJsonPath);
    if (!inputFile.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(inputFile.readAll());
    inputFile.close();

    // Navigate to the words array: results -> channels[0] -> alternatives[0] -> words
    QJsonObject rootObj = doc.object();
    QJsonObject results = rootObj.value("results").toObject();
    QJsonArray channels = results.value("channels").toArray();

    if (channels.isEmpty()) return;

    QJsonObject firstChannel = channels.at(0).toObject();
    QJsonArray alternatives = firstChannel.value("alternatives").toArray();

    if (alternatives.isEmpty()) return;

    QJsonObject firstAlt = alternatives.at(0).toObject();
    QJsonArray words = firstAlt.value("words").toArray();

    int segmentDurationMs = clipSec * 1000;
    int currentSegmentIndex = -1;
    QJsonArray currentSegmentWords;

    auto saveCurrentSegment = [&]() {
        if (currentSegmentWords.isEmpty()) return;

        QString fileName = QString("segment_%1.json").arg(currentSegmentIndex, 3, 10, QChar('0'));
        QFile outFile(QDir(outputDir).filePath(fileName));
        if (outFile.open(QIODevice::WriteOnly)) {
            QJsonObject root;
            root.insert("words", currentSegmentWords);
            outFile.write(QJsonDocument(root).toJson());
            outFile.close();
        }

        currentSegmentWords = QJsonArray();
    };

    for (const QJsonValue &wordValue : qAsConst(words))
    {
        QJsonObject wordObj = wordValue.toObject();
        double startSec = wordObj["start"].toDouble();
        int startMs = static_cast<int>(startSec * 1000);

        int segmentIndex = startMs / segmentDurationMs;
        int offsetMs = segmentIndex * segmentDurationMs;

        // If we move to a new segment, save the previous one
        if (segmentIndex != currentSegmentIndex)
        {
            saveCurrentSegment();
            currentSegmentIndex = segmentIndex;
        }

        // Adjust timestamps relative to the segment start (Parity with VTT logic)
        QJsonObject adjustedWord = wordObj;
        adjustedWord.insert("start_relative", (startMs - offsetMs) / 1000.0);
        adjustedWord.insert("end_relative", (static_cast<int>(wordObj["end"].toDouble() * 1000) - offsetMs) / 1000.0);

        currentSegmentWords.append(adjustedWord);
    }

    // Save the final segment
    saveCurrentSegment();

    // Generate the JSON manifest for these segments
    this->generateJsonManifest(outputDir, clipSec);
}

void AudioTranscriptionController::generateJsonManifest(const QString &outputDir, qint32 clipSec)
{
    QDir dir(outputDir);
    // Filter for the specific segment naming pattern: segment_***.json
    QStringList filters;
    filters << "segment_*.json";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Unsorted);

    QMap<qint32, QFileInfo> sortedFileMap;
    QRegularExpression regex("segment_(\\d+)\\.json"); // Relaxed to \\d+ to support varying padding digits safely

    for (const QFileInfo &fileInfo : qAsConst(fileList))
    {
        QRegularExpressionMatch match = regex.match(fileInfo.fileName());

        if (match.hasMatch())
        {
            qint32 segmentIndex = match.captured(1).toInt();
            sortedFileMap.insert(segmentIndex, fileInfo);
        }
    }

    QJsonArray segmentsArray;
    int totalSegments = sortedFileMap.size();

    // Iterate through the numerically sorted collection
    QMap<qint32, QFileInfo>::const_iterator cit = sortedFileMap.constBegin();
    qint32 segmentIndex;

    while (cit != sortedFileMap.constEnd())
    {
        segmentIndex = cit.key();
        const QFileInfo &fileInfo = cit.value();

        int startTime = segmentIndex * clipSec;
        int endTime = startTime + clipSec;

        QJsonObject segmentObj;
        segmentObj.insert("segment_index", segmentIndex);
        segmentObj.insert("file_name", fileInfo.fileName());
        segmentObj.insert("file_path", fileInfo.absoluteFilePath());

        QJsonObject timing;
        timing.insert("startTimeSec", startTime);
        timing.insert("endTimeSec", endTime);
        segmentObj.insert("timing", timing);

        segmentsArray.append(segmentObj);
        ++cit;
    }

    QJsonObject root;
    root.insert("exportTimestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    root.insert("segmentDurationSec", clipSec);
    root.insert("totalSegments", totalSegments);
    root.insert("segments", segmentsArray);

    // Save the manifest file
    QString manifestPath = dir.filePath("deepgram_segmentation_manifest.json");

    QFile outFile(manifestPath);

    if (outFile.open(QIODevice::WriteOnly))
    {
        outFile.write(QJsonDocument(root).toJson());
        outFile.close();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Deepgram JSON manifest generated at: %1")
                              .arg(manifestPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        FileInfo mFileInfo(manifestPath, FileType_t::AnnotationManifest, true);

        emit manifestReady(mFileInfo);
    }
}
