
#include <QObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QThread>

#include "inc/AudioModel.h"
#include "inc/AudioSegmentationController.h"
#include "inc/AudioSegmentationWorker.h" // The concrete implementation we built previously

#include "inc/LogUtils.h"
#include "inc/ExtractorUtils.h"

AudioSegmentationController::AudioSegmentationController(PipelineWorker *parent) :
    AudioSegmentationController(QString(),
                                  QString(),
                                  *Constants::Q_QGS_DEFAULT_CLIP_SEC,
                                  parent) // Default to 5 minutes
{
    // TODO
}

AudioSegmentationController::AudioSegmentationController(const QString & input,
                                                         const QString & dir,
                                                         const qint32& clipSec,
                                                         PipelineWorker *parent) :
    PipelineWorker(parent),
    m_workerThread(nullptr),
    m_worker(nullptr),
    m_inputAudio(input),
    m_outputDir(dir),
    m_clipSec(clipSec),
    m_audioModel(QSharedPointer<AudioModel>::create()),
    m_lastProcessingDurationMs(0)
{
    this->m_status.clear();
    this->updateView();
}

AudioSegmentationController::~AudioSegmentationController()
{
    if (m_workerThread && this->m_workerThread->isRunning())
    {
        this->m_workerThread->requestInterruption();
        this->m_workerThread->quit();

        if (!this->m_workerThread->wait(3000))
        {
            this->m_workerThread->terminate();
            this->m_workerThread->wait();
        }
    }
}

bool AudioSegmentationController::operator ==(const AudioSegmentationController & rhs) const
{
    if ((this->m_inputAudio != rhs.m_inputAudio) ||
        (this->m_outputDir != rhs.m_outputDir) ||
        (!qFuzzyCompare(this->m_clipSec + 1.00, rhs.m_clipSec + 1.00)) ||
        (this->m_status != rhs.m_status) ||
        (this->m_lastProcessingDurationMs == rhs.m_lastProcessingDurationMs))
    {
        return false;
    }

    if (this->m_audioModel == rhs.m_audioModel) return true;
    if (this->m_audioModel.isNull() || rhs.m_audioModel.isNull()) return false;

    // Assuming AudioSegmentModel has a segments() getter like VideoSegmentModel
    return (this->m_audioModel->segments() == rhs.m_audioModel->segments());
}

bool AudioSegmentationController::operator !=(const AudioSegmentationController & right) const
{
    return !(*this == right);
}

Url AudioSegmentationController::inputAudio() const
{
    return this->m_inputAudio;
}

bool AudioSegmentationController::setInputAudio(const Url  &newInputAudio)
{
    if (newInputAudio.isEmpty() || (this->m_inputAudio == newInputAudio)) return false;
    this->m_inputAudio = newInputAudio;
    return true;
}

bool AudioSegmentationController::setInputAudio(const QString & newInputAudio)
{
    if (newInputAudio.isEmpty()) return false;

    Url url(newInputAudio);

    return this->setInputAudio(url);
}

Url AudioSegmentationController::outputDir() const
{
    return this->m_outputDir;
}

bool AudioSegmentationController::setOutputDir(const Url &newOutputDir)
{
    if (newOutputDir.isEmpty() || (newOutputDir == this->m_outputDir)) return false;
    this->m_outputDir = newOutputDir;
    return true;
}

bool AudioSegmentationController::setOutputDir(const QString &newOutputDir)
{
    if (newOutputDir.isEmpty()) return false;

    Url url(newOutputDir);

    return this->setOutputDir(url);
}

qreal AudioSegmentationController::clipSec() const
{
    return this->m_clipSec;
}

bool AudioSegmentationController::setClipSec(const qreal newClipSec)
{
    if (newClipSec <= 0.00 || qFuzzyCompare(newClipSec, this->m_clipSec)) return false;
    this->m_clipSec = newClipSec;
    return true;
}

AudioModel* AudioSegmentationController::audioModel() const
{
    return this->m_audioModel.data();
}

QSharedPointer<AudioModel> AudioSegmentationController::audioModelClone() const
{
    return this->m_audioModel->clone();
}

QString AudioSegmentationController::status() const
{
    return this->m_status;
}

void AudioSegmentationController::setupAudioCutter(const QString & inputAudio,
                                                   const QString & segment_dir,
                                                   const qint32 & clipSec)
{
    this->setInputAudio(inputAudio);
    this->setOutputDir(segment_dir);
    this->setClipSec(clipSec);
}

void AudioSegmentationController::clearResults()
{
    if (this->audioModel()) {
        this->audioModel()->clear();
    }
    this->setStatus(tr("Ready"));
    emit this->progressUpdated(0);
}

void AudioSegmentationController::cancelSegmentation()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        this->setStatus("Aborting and cleaning up...");
        emit requestAbort();
    }
}

void AudioSegmentationController::setStatus(const QString &newStatus)
{
    if (!newStatus.isEmpty() && newStatus != this->m_status)
    {
        this->m_status = newStatus;
        emit statusChanged(newStatus);
    }
}

void AudioSegmentationController::handleInput(FileInfo info)
{
    // 1. Check existing process
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        emit error("Segmentation already in progress.");
        return;
    }

    if (info.url.isEmpty() || ! info.url.isValid())
    {
        emit error("Input audio file path is invalid.");

        return;
    }

    // 2. Validate Environment (Checking file existence and directory write access)
    if (!QFile::exists(info.url.path()))
    {
        emit error("Input audio file does not exist.");

        return;
    }

    this->setInputAudio(info.url);

    QDir outDir(this->outputDir().path());

    if (!outDir.exists() && !outDir.mkpath("."))
    {
        emit error("Output directory cannot be created.");
        return;
    }

    // 3. Clear existing results
    if (this->audioModel())
    {
        this->clearResults();
    }

    // 4. Instantiate Concrete Worker directly (No Factory needed for purely local FFmpeg audio)
    this->m_worker = new AudioSegmentationWorker(this->inputAudio().path(), this->outputDir().path(), this->clipSec(), nullptr);

    if (!this->m_worker)
    {
        emit error("Failed to create Audio worker.");
        return;
    }

    this->m_workerThread = new QThread;
    this->m_worker->moveToThread(this->m_workerThread);

    // 5. Connect Signals
    QObject::connect(this->m_worker,
                     &IAudioSegmentationWorker::progressUpdated,
                     this,
                     &AudioSegmentationController::progressUpdated);

    QObject::connect(this->m_worker,
                     &IAudioSegmentationWorker::statusUpdated,
                     this,
                     &AudioSegmentationController::setStatus);

    QObject::connect(this->m_worker,
                     &IAudioSegmentationWorker::error,
                     this,
                     &AudioSegmentationController::handleError);

    if (this->m_audioModel)
    {
        QObject::connect(this->m_worker,
                         &IAudioSegmentationWorker::resultReady,
                         this->m_audioModel.data(),
                         &AudioModel::updateResults);
    }

    // QObject::connect(this->m_worker,
    //                  &IAudioSegmentationWorker::finished,
    //                  this,
    //                  [this](qint32 exit, qint32 status){
    //                      this->handleFinished(exit, status);
    //                      this->m_workerThread->quit(); // Now it's safe to quit
    //                  });

    QObject::connect(this->m_worker,
                     &AudioSegmentationWorker::finished,
                     this,
                     &AudioSegmentationController::handleFinished,
                     Qt::QueuedConnection);

    // Abort routing
    QObject::connect(this,
                     &AudioSegmentationController::requestAbort,
                     this->m_worker,
                     &IAudioSegmentationWorker::abort,
                     Qt::DirectConnection);

    // Thread Lifecycle
    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this->m_worker,
                     &IAudioSegmentationWorker::process);

    // QObject::connect(this->m_worker,
    //                  &IAudioSegmentationWorker::finished,
    //                  this->m_workerThread,
    //                  &QThread::quit);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_worker,
                     &QObject::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_workerThread,
                     &QObject::deleteLater);

    // 6. Start Processing
    this->m_timer.start();
    this->m_workerThread->start();
    this->m_workerThread->setPriority(QThread::LowPriority);
}

void AudioSegmentationController::handleFinished(qint32 exitCode, qint32 status)
{
#ifdef QT_DEBUG
    LogUtils::safeLog("AudioSegmentationController::handleFinished() - Starting!",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    qint64 elapsedMs = m_timer.elapsed();
    this->m_lastProcessingDurationMs = elapsedMs;
    QString timeStr = ExtractorUtils::formatTime(this->m_lastProcessingDurationMs);

    QString finalStatus;
    bool success = false;

    if (exitCode == 0 && status == 0)
    {
        success = true;
        finalStatus = tr("Processing completed successfully in %1").arg(timeStr);

    } else if (exitCode == -1313558101) { // AVERROR_EXIT equivalent
        finalStatus = tr("Processing aborted by user after %1").arg(timeStr);

    } else {
        finalStatus = tr("Processing failed after %1 (Code: %2)").arg(timeStr).arg(exitCode);
    }

    this->setStatus(finalStatus);

    if (exitCode != 0)
    {
        this->cancelSegmentation(); // Only abort/cleanup if it didn't finish normally
    }

    if (success && this->audioModel())
    {
#ifdef QT_DEBUG
        int count = this->audioModel()->rowCount();

        LogUtils::safeLog(QString("AudioSegmentationController::handleFinished() - Final audio segment count: %1")
                              .arg(count),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    Audio::AudioList_t results = this->getSegmentsMetadata(this->outputDir());

    // Export the structured data alongside the audio segments
    QString jsonPath = QDir(this->outputDir().path()).absoluteFilePath("audio_segmentation_manifest.json");

    // Assuming you have an equivalent Utils export function for AudioList_t
    if (this->exportAudioSegmentsToJson(results, jsonPath, this->inputAudio().path()))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("AudioSegmentationWorker::handleFinished() - Audio manfist created: %1")
                              .arg(jsonPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        FileInfo audioManifestFileInfo(jsonPath, FileType_t::AudioManifest, true);

        emit manifestReady(audioManifestFileInfo);
    }

    emit statusChanged(finalStatus);

    FileInfo audioSegsDirInfo(this->outputDir(), FileType_t::AudioSegmentsDir);
    emit finished(audioSegsDirInfo);

    this->cancelSegmentation();
}

void AudioSegmentationController::handleError(QString msg)
{
    emit error(msg);
    emit statusChanged("Segmentation failed: " + msg);
    this->cancelSegmentation();
}

bool AudioSegmentationController::exportAudioSegmentsToJson(const Audio::AudioList_t &results,
                                                            const QString &outputPath,
                                                            const QString &sourceAudioName)
{
#ifdef QT_DEBUG
    LogUtils::safeLog("AudioSegmentationController::exportAudioSegmentsToJson(...) - Starting.",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (results.isEmpty() || outputPath.isEmpty())
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("AudioSegmentationController::exportAudioSegmentsToJson(...) - results or outputPath is empty.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return false;
    }

    QJsonArray segmentsArray;

    // Leverage the Audio::toJson() method
    for (const auto &info : results)
    {
        segmentsArray.append(info.toJson());
    }

    // Root Document Metadata (Crucial for Provenance)
    QJsonObject rootObj;
    rootObj["sourceAudio"] = sourceAudioName;
    rootObj["exportTimestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    rootObj["totalSegments"] = results.size();
    rootObj["segments"] = segmentsArray;

    QJsonDocument doc(rootObj);

    // Write to Disk
    QFile file(outputPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("AudioSegmentationController::exportAudioSegmentsToJson(...) - Failed to open JSON export file for writing.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return false;
    }

    // Use Indented format for human-readability during debugging and reporting
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioSegmentationController::exportAudioSegmentsToJson(...) - JSON export success.",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return true;
}

void AudioSegmentationController::updateView()
{
    // Reserved for UI table model synchronization
}


// --- Metadata Extraction ---

Audio::AudioList_t AudioSegmentationController::getSegmentsMetadata(const Url & segment_dir)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioSegmentationWorker::getSegmentsMetadata(...) - Starting: %1!")
                      .arg(segment_dir.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QDir segmentQDir(segment_dir.path());
    QString filter = "segment_*.mp3"; // Matching the FFmpeg pipeline output
    QStringList segmentFiles = segmentQDir.entryList({filter}, QDir::Files, QDir::Unsorted);

    double nominalDuration = static_cast<double>(this->clipSec());
    QRegularExpression re("segment_(\\d+)\\.mp3");

    // --- Map files numerically by their parsed index to fix alphabetical sort vulnerability ---
    QMap<qint32, QString> sortedSegmentMap;

    for (const QString &fileName : qAsConst(segmentFiles))
    {
        QRegularExpressionMatch match = re.match(fileName);

        if (match.hasMatch())
        {
            qint32 segmentIdx = match.captured(1).toInt();
            sortedSegmentMap.insert(segmentIdx, fileName);
        }
    }

    Audio::AudioList_t results;

    QMap<qint32, QString>::const_iterator cit = sortedSegmentMap.constBegin();
    qint32 segmentIdx;
    qint32 missingIdx;
    QString fileName;

    while (cit != sortedSegmentMap.constEnd())
    {
        segmentIdx = cit.key();
        fileName = cit.value();

        // 1. Fill any missing segments (Gaps) caused by FFmpeg drops
        while (results.size() < segmentIdx)
        {
            missingIdx = results.size();
            double expectedStartTime = missingIdx * nominalDuration;

            Audio gap;
            gap.setSegmentIndex(missingIdx);
            gap.setTiming(expectedStartTime, expectedStartTime + nominalDuration);
            gap.setState("GAP");

            QByteArray gapSeed = QString("AUDIO_GAP_%1_%2").arg(missingIdx).arg(nominalDuration).toUtf8();
            gap.setId(UniqueId(gapSeed));

            results.append(gap);
        }

        // 2. Validate current segment
        QString fullPath = segmentQDir.absoluteFilePath(fileName);

        // Ensure the MP3 actually has audio data and isn't just a 0-byte shell
        double actualDuration = this->m_worker->getDurationViaFFprobe(fullPath);
        bool isValid = (actualDuration > 0.0);

        double expectedStartTime = segmentIdx * nominalDuration;

        Audio info;
        info.setTiming(expectedStartTime, expectedStartTime + actualDuration);

        if (isValid)
        {
            info.setFilePath(fullPath);
            info.setState("OK");

            // Generate deterministic ID based on the binary content of the MP3
            QByteArray contentSeed = ExtractorUtils::generateContentSeed(fullPath, QCryptographicHash::Md4);

            if (!contentSeed.isNull() && !contentSeed.isEmpty())
            {
                info.setId(UniqueId(contentSeed));
            } else {
                // Fallback ID based on the sequence index
                QRegularExpressionMatch match = re.match(fileName);
                QString fallbackString = match.hasMatch() ? match.captured(1) : QString("audio_%1").arg(segmentIdx, 3, 10, QChar('0'));
                info.setId(UniqueId(fallbackString.toUtf8()));
            }

            info.setSegmentIndex(segmentIdx);

        } else {
            // Handle Corrupt Segments
            info.setFilePath(QString());
            info.setState("CORRUPT");
            info.setId(UniqueId(fileName.toUtf8()));

            QFile::remove(fullPath);

#ifdef QT_DEBUG
            LogUtils::safeLog("Removing invalid audio segment: " + fileName,
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

        results.append(info);

        ++cit;
    }

    // Optional: Assuming your Utils class handles Identity Sanity Checks for Audio as well
    // Utils::performIdentitySanityCheck(results);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioSegmentationController::getSegmentsMetadata(...) - Finishing with error: %1")
                      .arg(results.size()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return results;
}
