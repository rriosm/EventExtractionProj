
#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QThread>
//#include <QTableView>

#include "VideoSegmentModel.h"
#include "VideoSegmentationController.h"
// #include "VideoSegmentationWorker.h"
// #include "VideoSegmentationWorkerCLI.h"
#include "VideoSegmentationWorkerFactory.h"

#include "LogUtils.h"
#include "ExtractorUtils.h"

VideoSegmentationController::VideoSegmentationController(PipelineWorker *parent) :
    VideoSegmentationController(QString(),
                                  ModelCode_t::None,
                                  QString(),
                                  0,
                                  SegmentationMode_t::UseLibrary,
                                  parent)
{
    // TODO
}

VideoSegmentationController::VideoSegmentationController(const QString & input,
                                                         const ModelCode_t & model,
                                                         const QString & dir,
                                                         const qreal& clipSec,
                                                         const SegmentationMode_t segModeCode,
                                                         PipelineWorker *parent) :
    PipelineWorker(parent),
    m_workerThread(nullptr),
    m_worker(nullptr),
    m_inputVideo(input),
    m_model(model),
    m_dir(dir),
    m_clipSec(clipSec),
    m_videoSegModel(QSharedPointer<VideoSegmentModel>::create()),
    m_lastProcessingDurationMs(0),
    m_segModeCode(segModeCode),
    m_slowdown_factor(1.0)
{
    this->m_status.clear();
    this->updateView();
}

VideoSegmentationController::~VideoSegmentationController()
{
    if (m_workerThread && this->m_workerThread->isRunning())
    {
        this->m_workerThread->requestInterruption(); // Tell the curl callback to return 1
        this->m_workerThread->quit();

        if ( ! this->m_workerThread->wait(3000) ) // Wait up to 3 seconds
        {
            this->m_workerThread->terminate();  // Force kill if it hangs
            this->m_workerThread->wait();       // Still wait to ensure cleanup
        }
    }
}

bool VideoSegmentationController::operator ==(const VideoSegmentationController & rhs) const
{
    if ( (this->m_inputVideo != rhs.m_inputVideo) ||
         (this->m_model != rhs.m_model) ||
         (this->m_dir != rhs.m_dir) ||
         (!qFuzzyCompare(this->m_clipSec + 1.00, rhs.m_clipSec + 1.00)) ||
         (!qFuzzyCompare(this->m_slowdown_factor + 1.00, rhs.m_slowdown_factor + 1.00)) ||
         (this->m_status != rhs.m_status) ||
         (this->m_lastProcessingDurationMs == rhs.m_lastProcessingDurationMs) )
    {
        return false;
    }

    if (this->m_videoSegModel == rhs.m_videoSegModel) return true;

    if (this->m_videoSegModel.isNull() || rhs.m_videoSegModel.isNull()) return false;

    return (this->m_videoSegModel->segments() == rhs.m_videoSegModel->segments());
}

bool VideoSegmentationController::operator !=(const VideoSegmentationController & right) const
{
    return !(*this == right);
}

bool VideoSegmentationController::operator==(const VideoSegmentationController *ptr) const
{
    return (this == ptr);
}

bool VideoSegmentationController::operator!=(const VideoSegmentationController *ptr) const
{
    return (this != ptr);
}

QString VideoSegmentationController::inputVideo() const
{
    return this->m_inputVideo;
}
bool VideoSegmentationController::setInputVideo(const QString &newInputVideo)
{
    if (this->m_inputVideo == newInputVideo) return false;

    this->m_inputVideo = newInputVideo;

    return true;
}

ModelCode_t VideoSegmentationController::modelCode() const
{
    return this->m_model;
}

bool VideoSegmentationController::setModelCode(const ModelCode_t newModel)
{
    if ( (newModel == ModelCode_t::None) ||
         (newModel == this->m_model) )
    {
        return false;
    }

    this->m_model = newModel;

    return true;
}

QString VideoSegmentationController::outputDir() const
{
    return this->m_dir;
}
bool VideoSegmentationController::setOutputDir(const QString &newDir)
{
    if ( newDir.isEmpty() ||
         (newDir == this->m_dir) )
    {
        return false;
    }

    this->m_dir = newDir;

    return true;
}

qreal VideoSegmentationController::clipSec() const
{
    return this->m_clipSec;
}

bool VideoSegmentationController::setClipSec(const qreal newClipSec)
{
    if ( newClipSec <= 0.00 || qFuzzyCompare(newClipSec, this->m_clipSec)) return false;

    this->m_clipSec = newClipSec;

    return true;
}

qreal VideoSegmentationController::slowdown_factor() const
{
    return this->m_slowdown_factor;
}

void VideoSegmentationController::setSlowdown_factor(qreal newSlowdown_factor)
{
    if (qFuzzyCompare(newSlowdown_factor, this->m_slowdown_factor)) return;

    this->m_slowdown_factor = newSlowdown_factor;
}


QString VideoSegmentationController::status()
{
    return this->m_status;
}

VideoSegmentModel * VideoSegmentationController::videoSegModel() const
{
    return this->m_videoSegModel.data();
}

QSharedPointer<VideoSegmentModel> VideoSegmentationController::videoSegModelClone() const
{
    QSharedPointer<VideoSegmentModel> ptr1 = this->m_videoSegModel->clone();

    return ptr1;
}

bool VideoSegmentationController::setSegModeCode(const SegmentationMode_t newSegModeCode)
{
    if ( (newSegModeCode == SegmentationMode_t::None) ||
         (newSegModeCode == this->m_segModeCode) )
    {
        return false;
    }

    this->m_segModeCode = newSegModeCode;

    return true;
}

SegmentationMode_t VideoSegmentationController::segModeCode() const
{
    return this->m_segModeCode;
}

void VideoSegmentationController::setupVideoCutter(const ModelCode_t & modelCode,
                                                   const QString & segment_dir,
                                                   const qint32 & clipSec,
                                                   const qreal & slowdonwFactor,
                                                   const SegmentationMode_t segModeCode)
{
    this->setModelCode(modelCode);
    this->setOutputDir(segment_dir);
    this->setClipSec(clipSec);
    this->setSlowdown_factor(slowdonwFactor);
    this->setSegModeCode(segModeCode);
}

void VideoSegmentationController::handleInput(FileInfo info)
{
    // 1. Check existing process
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        emit error("Segmentation already in progress.");
        return;
    }

    if (info.url.isEmpty() || ! info.url.isValid())
    {
        emit error("Input file is empty or invalid.");
        return;
    }

    if (info.type != FileType_t::VideoFile)
    {
        emit error("Invalid input file type.");

        return;
    }

    this->setInputVideo(info.url.path());

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 01",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QString errorMsg;
    // 1. Environment & Fallback Logic
    if ( ! VideoSegmentationWorkerFactory::verifyEnvironment(this->segModeCode(), this->outputDir(), this->inputVideo(), errorMsg) )
    {
        if (this->segModeCode() == SegmentationMode_t::UseLibrary)
        {
#ifdef QT_DEBUG
            LogUtils::safeLog("Library Mode (Lib) check failed: " + errorMsg + ". Attempting CLI fallback...",
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif

            this->setSegModeCode(SegmentationMode_t::UseCLI);

            if ( ! VideoSegmentationWorkerFactory::verifyEnvironment(this->segModeCode(), this->outputDir(), this->inputVideo(), errorMsg) )
            {
                emit error("Critical Failure: No valid FFmpeg (CLI) found. " + errorMsg);
                return;
            }
        } else {
            emit error("Critical Failure: CLI environment check failed. " + errorMsg);
            return;
        }
    }

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 02",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Prepare the Model (Assuming m_videoSegModel is a member of VideoProcessor)
    // Clear the UI table before the background thread starts
    if (this->videoSegModel())
    {
        this->clearResults();
    }

//    // 1. Create worker and thread
//    VideoSegmentationWorkerCLI * worker = new VideoSegmentationWorkerCLI(
//                                      this->sharedDataP->inputVideo(),
//                                      this->sharedDataP->dir(),
//                                      this->sharedDataP->model(),
//                                      this->sharedDataP->clipSec() );

    // A QObject cannot be moved to a different thread if it has a parent. Then, use nullptr
    this->m_worker = VideoSegmentationWorkerFactory::createWorker(this->segModeCode(), nullptr);

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 03",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if ( ! this->m_worker )
    {
        emit error("Failed to create worker.");
        return;
    }

    this->m_worker->setInputVideo(this->inputVideo());
    this->m_worker->setDir(this->outputDir());
    this->m_worker->setModelCode(this->modelCode());
    this->m_worker->setClipSec(this->clipSec());
    this->m_worker->setSlowdownFactor(this->slowdown_factor());

    this->m_workerThread = new QThread;

    this->m_worker->moveToThread(this->m_workerThread);

    // Connect worker signals to VideoProcessor signals (proxying)
    QObject::connect(this->m_worker,
                     &IVideoSegmentationWorker::progressUpdated,
                     this,
                     &VideoSegmentationController::progressUpdated);

    QObject::connect(this->m_worker,
                     &IVideoSegmentationWorker::statusUpdated,
                     this,
                     &VideoSegmentationController::setStatus);

    QObject::connect(this->m_worker,
                     &IVideoSegmentationWorker::error,
                     this,
                     &VideoSegmentationController::handleError);

    // Connect resultReady to your UI finished signal and the Model
    // Assuming VideoSegmentModel is your Table Model

    if (this->videoSegModel())
    {
//                QTableView tableView->setModel(videoSegModel);

        QObject::connect(this->m_worker,
                         &IVideoSegmentationWorker::resultReady,
                         this->m_videoSegModel.data(),
                         &VideoSegmentModel::updateResults);
    }

    QObject::connect(this->m_worker,
                     &IVideoSegmentationWorker::finished,
                     this,
                     &VideoSegmentationController::handleFinished);

    // Connect the Processor's abort signal to the Worker's abort slot
    // Abort Handling (DirectConnection is vital here)
    QObject::connect(this,
                     &VideoSegmentationController::requestAbort,
                     this->m_worker,
                     &IVideoSegmentationWorker::abort,
                     Qt::DirectConnection);

    // Lifecycle Management
    // Setup Signal Routing
    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this->m_worker,
                     &IVideoSegmentationWorker::process);

    // Clean up (The "Golden Rule" of QThread cleanup)
    QObject::connect(this->m_worker,
                     &IVideoSegmentationWorker::finished,
                     this->m_workerThread,
                     &QThread::quit);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_worker,
                     &IVideoSegmentationWorker::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_workerThread,
                     &QThread::deleteLater);

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 04",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->m_timer.start();

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 05",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->m_workerThread->start();

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 06",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Set the priority of the thread after it has started
    // Options: IdlePriority, LowestPriority, LowPriority, NormalPriority, HighPriority
    this->m_workerThread->setPriority(QThread::LowPriority);

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationController::handleInput(...) -- 07",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

void VideoSegmentationController::clearResults()
{
    if (this->videoSegModel())
    {
        this->videoSegModel()->clear();
    }

    this->setStatus(tr("Ready"));
    emit this->progressUpdated(0);
}

void VideoSegmentationController::cancelSegmentation()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        // Emitting this signal will trigger the worker's abort() slot
        // across the thread boundary safely.

        this->setStatus("Aborting and cleaning up...");

        emit requestAbort();

        // Set connect to IVideoSegmentationWorker to alert controller to finish!
//        this->m_workerThread->requestInterruption();
    }
}

void VideoSegmentationController::setStatus(const QString &newStatus)
{
    if ( ! newStatus.isEmpty() && newStatus != this->m_status)
    {
        this->m_status = newStatus;

        emit statusChanged(newStatus);

#ifdef QT_DEBUG
        LogUtils::safeLog("Processor Status: " + newStatus,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
}

void VideoSegmentationController::handleFinished(qint32 exitCode, qint32 status)
{
    // 1. Capture the total time elapsed from the class member timer
    qint64 elapsedMs = m_timer.elapsed();
    this->m_lastProcessingDurationMs = elapsedMs;
    QString timeStr = ExtractorUtils::formatTime(this->m_lastProcessingDurationMs);

    QString finalStatus;
    bool success = false;

    // 1. Evaluate the results based on FFmpeg and Internal status
    if (exitCode == 0 && status == 0)
    {
        // Absolute Success
        success = true;
        finalStatus = tr("Processing completed successfully in %1").arg(timeStr);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Pipeline finished: Success. Total time: %1").arg(timeStr),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }
    else if (exitCode == -1313558101) // Corresponds to AVERROR_EXIT (Abort)
    {
        finalStatus = tr("Processing aborted by user after %1").arg(timeStr);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Pipeline finished: User Aborted at %1").arg(timeStr),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    } else {
        // Failure - we retrieve the last error if possible, or use a generic one
        finalStatus = tr("Processing failed after %1 (Code: %2, Internal: %3)")
                        .arg(timeStr)
                        .arg(exitCode)
                        .arg(status);

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Pipeline finished: Error at %1. ExitCode: %2, Status: %3")
                          .arg(timeStr).arg(exitCode).arg(status),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // 2. Update the Shared Data Status
    // This will trigger the d-pointer's detach if the data is shared elsewhere
    this->setStatus(finalStatus);

    // 3. Optional: Finalize the Model
    // If you want to sort the results or perform a final validation
    // you would do it here before the UI is considered "Done"
    if (success && this->videoSegModel())
    {
        // Example: Log how many segments were actually found
        qint32 count = this->videoSegModel()->rowCount();

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Final segment count: %1").arg(count),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    VideoSegmentInfo::VideoSegmentInfoList_t results = this->getSegmentsMetadata(this->modelCode(), this->outputDir());

    // Export the structured data alongside the video segments
    QString jsonPath = QDir(this->outputDir()).absoluteFilePath("video_segmentation_manifest.json");

    if (this->exportVideoSegmentsToJson(results, jsonPath, this->inputVideo()))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("VideoSegmentationController::handleFinished() - Video manfist created: %1")
                              .arg(jsonPath),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        FileInfo videoManifestFile(jsonPath, FileType_t::VideoManifest, true);

        emit manifestReady(videoManifestFile);
    }

    // 4. Notify the UI / Application
    emit statusChanged(finalStatus);

    // This is the signal main.cpp or UI is waiting for to unlock buttons
    FileInfo videoSegsDirInfo(this->outputDir(), FileType_t::VideoSegmentsDir, true);

    emit finished(videoSegsDirInfo);

    this->cancelSegmentation();
}

void VideoSegmentationController::handleError(QString msg)
{
    // 1. Notify the UI via the public signal
    emit error(msg);

    emit statusChanged("Decoupage failed: " + msg);

    // 2. Stop the thread
    this->cancelSegmentation();
}

bool VideoSegmentationController::exportVideoSegmentsToJson(const VideoSegmentInfo::VideoSegmentInfoList_t &results,
                                                            const QString &outputPath,
                                                            const QString &sourceVideoName,
                                                            quint16 generatorVersion)
{
    if (results.isEmpty() || outputPath.isEmpty()) return false;

    QJsonArray segmentsArray;

    // Leverage the refactored VideoSegmentInfo::toJson() method
    for (const auto &info : results)
    {
        segmentsArray.append(info.toJson());
    }

    // Root Document Metadata (Crucial for Provenance)
    QJsonObject rootObj;
    rootObj["sourceVideo"] = sourceVideoName;
    rootObj["exportTimestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    rootObj["totalSegments"] = results.size();
    rootObj["generatorVersion"] = QString::number(generatorVersion); // Matching the version of the class that executes this function
    rootObj["segments"] = segmentsArray;

#ifdef QT_DEBUG
    LogUtils::safeLog("Utils::exportVideoSegmentsToJson(...) -- rootObj created.",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QJsonDocument doc(rootObj);

    // Write to Disk
    QFile file(outputPath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("Failed to open JSON export file for writing.",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        return false;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Utils::exportVideoSegmentsToJson(...) -- %1 created and opened.")
                          .arg(outputPath),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Use Indented format for human-readability during debugging and reporting
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Utils::exportVideoSegmentsToJson(...) -- %1 closed.")
                          .arg(outputPath),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return true;
}

VideoSegmentInfo::VideoSegmentInfoList_t VideoSegmentationController::getSegmentsMetadata(const ModelCode_t & modelCode, const QString & segment_dir)
{
    VideoModelConfig cfg = ExtractorUtils::getConfig(modelCode);

    if ( ! cfg.isValid() )
    {
        qCritical() << "Unsupported model code:" << static_cast<qint32>(modelCode);
        return {};
    }

    QDir segmentQDir(segment_dir);
    QString suffix = cfg.suffix();
    QString filter = QString("segment%1_*.mp4").arg(suffix);
    QStringList segmentFiles = segmentQDir.entryList({filter}, QDir::Files, QDir::Unsorted);

    double nominalDuration = static_cast<double>(this->clipSec());
    QRegularExpression re(QString("segment%1_(\\d+)\\.mp4").arg(suffix));

    // Map files numerically by index to fix alphabetical sorting bug ---
    QMap<qint32, QString> sortedSegmentMap;

    for (const QString &fileName : qAsConst(segmentFiles))
    {
        QRegularExpressionMatch match = re.match(fileName);

        if (match.hasMatch())
        {
            quint32 segmentIdx = match.captured(1).toUInt();
            sortedSegmentMap.insert(segmentIdx, fileName);
        }
    }

    VideoSegmentInfo::VideoSegmentInfoList_t results;
    qint32 segmentIdx;
    qint32 missingIdx;
    QString fileName;

    // Iterate through the numerically sorted map
    QMap<qint32, QString>::const_iterator cit = sortedSegmentMap.constBegin();

    while (cit != sortedSegmentMap.constEnd())
    {
        segmentIdx = cit.key();
        fileName = cit.value();

        QRegularExpressionMatch match = re.match(fileName);

        while (results.size() < segmentIdx)
        {
            missingIdx = results.size();
            VideoSegmentInfo gap = ExtractorUtils::createGapEntry(missingIdx, nominalDuration);

            QByteArray gapSeed = QString("GAP_%1_%2_%3").arg(suffix).arg(missingIdx).arg(nominalDuration).toUtf8();
            gap.setId(UniqueId(gapSeed));
            gap.setSegmentIndex(missingIdx);

            results.append(gap);
        }

        QString fullPath = segmentQDir.absoluteFilePath(fileName);
        bool isValid = ExtractorUtils::isVideoSegmentValid(fullPath);
        double actualDuration = isValid ? this->m_worker->getDurationViaFFprobe(fullPath) : 0.0;
        double expectedStartTime = segmentIdx * nominalDuration;

        VideoSegmentInfo info;
        info.setTiming(expectedStartTime, expectedStartTime + actualDuration);
        info.setSlowdownFactor(this->slowdown_factor());

        if (isValid)
        {
            info.setFilePath(fullPath);
            info.setState("OK");

            QByteArray contentSeed = ExtractorUtils::generateContentSeed(fullPath, QCryptographicHash::Md4);

            if (!contentSeed.isNull() &&
                !contentSeed.isEmpty())
            {
                info.setId(UniqueId(contentSeed));

            } else {
                // Re-match for absolute fallback fallback string safety
                QRegularExpressionMatch match = re.match(fileName);
                QString fallbackString = match.hasMatch() ? match.captured(1) : QString("%1").arg(segmentIdx, 3, 10, QChar('0'));
                info.setId(UniqueId(fallbackString.toUtf8()));
            }

            info.setSegmentIndex(segmentIdx);

        } else {
            // Handle Corrupt
            info.setFilePath(QString());
            info.setState("CORRUPT");

            // Deterministic ID for corrupted segments based on filename
            info.setId(UniqueId(fileName.toUtf8()));

            QFile::remove(fullPath);

#ifdef QT_DEBUG
            LogUtils::safeLog("Removing invalid segment: " + fileName,
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

        results.append(info);
        ++cit;
    }

    // Validate identities before handing off to the application
    ExtractorUtils::performIdentitySanityCheck(results);

    return results;
}

void VideoSegmentationController::updateView()
{
    // Ensures the QTableView is looking at the correct model instance
//    if (ui->tableView && d->videoSegModel()) {
//        ui->tableView->setModel(d->videoSegModel());

//        // Optional: Adjust column widths to fit content
//        ui->tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    //    }
}
