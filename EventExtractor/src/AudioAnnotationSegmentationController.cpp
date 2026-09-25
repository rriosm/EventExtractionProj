
#include "inc/AudioAnnotationSegmentationController.h"

#include "inc/LogUtils.h"

AudioAnnotationSegmentationController::AudioAnnotationSegmentationController(QObject *parent) :
    PipelineWorker{parent},
    m_workerThread(nullptr),
    m_worker(nullptr),
    m_clipSec(0.00)
{
    this->m_annotationManifest.clear();
    this->m_annotationClipsOutputDir.clear();
}

AudioAnnotationSegmentationController::~AudioAnnotationSegmentationController()
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

bool AudioAnnotationSegmentationController::operator==(const AudioAnnotationSegmentationController &right) const
{
    if ((this->m_clipSec != right.m_clipSec) ||
        (this->m_annotationManifest != right.m_annotationManifest) ||
        (this->m_annotationClipsOutputDir != right.m_annotationClipsOutputDir) )
    {
        return false;
    }

    return true;
}

bool AudioAnnotationSegmentationController::operator!=(const AudioAnnotationSegmentationController &right) const
{
    return !(*this == right);
}

bool AudioAnnotationSegmentationController::operator==(const AudioAnnotationSegmentationController *ptr) const
{
    return (this == ptr);
}

bool AudioAnnotationSegmentationController::operator!=(const AudioAnnotationSegmentationController *ptr) const
{
    return (this != ptr);
}

qreal AudioAnnotationSegmentationController::clipSec() const
{
    return this->m_clipSec;
}

void AudioAnnotationSegmentationController::setClipSec(qreal newClipSec)
{
    if (newClipSec == this->m_clipSec) return;

    this->m_clipSec = newClipSec;
}

Url AudioAnnotationSegmentationController::annotationManifest() const
{
    return this->m_annotationManifest;
}

void AudioAnnotationSegmentationController::setAnnotationManifest(const Url &newAnnotationManifest)
{
    if (newAnnotationManifest == this->m_annotationManifest) return;

    this->m_annotationManifest = newAnnotationManifest;
}

QString AudioAnnotationSegmentationController::annotationClipsOutputDir() const
{
    return this->m_annotationClipsOutputDir;
}

void AudioAnnotationSegmentationController::setAnnotationClipsOutputDir(const QString &newAnnotationClipsOutputDir)
{
    if (newAnnotationClipsOutputDir == this->m_annotationClipsOutputDir) return;

    this->m_annotationClipsOutputDir = newAnnotationClipsOutputDir;
}

void AudioAnnotationSegmentationController::cancelSegmentation()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        QString errMsg = QString("Stopping audio annotation segmentation...");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioAnnotationSegmentationController::cancelSegmentation() --- " + errMsg,
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

void AudioAnnotationSegmentationController::handleInput(FileInfo info)
{
    QString errMsg;

    if (info.url.isEmpty() || ! info.url.isValid())
    {
        errMsg = QString("AudioAnnotationSegmentationController::handleInput(...) --- Audio input file is invalid!");

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
        errMsg = QString("Audio transcription segmentation is already in progress!");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioAnnotationSegmentationController::handleInput(...) --- " + errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(errMsg);

        return;
    }

    QFileInfo audioTranscriptionFile(info.url.path());

    if ( ! audioTranscriptionFile.isFile() || ! audioTranscriptionFile.exists() )
    {
        errMsg = QString("AudioAnnotationSegmentationController::handleInput(...) --- Transcription file is invalid!");

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
    this->m_worker = new AudioAnnotationSegmentationWorker;

    // 2. Configure the worker
    this->m_worker->setClipSec(this->clipSec());
    this->m_worker->setAnnotationManifest(this->annotationManifest());
    this->m_worker->setAnnotationClipsOutputDir(this->annotationClipsOutputDir());

    this->m_worker->moveToThread(this->m_workerThread);

    // 3. Setup Signal Forwarding
    // Progress: Map double (0.0-100.0) to int (0-100) for progress bar
    QObject::connect(this->m_worker,
                     &AudioAnnotationSegmentationWorker::progressUpdated,
                     this,
                     [this](double p){
                         emit progressChanged(static_cast<int>(p));
                     }, Qt::QueuedConnection);

    // 4. Error Handling
    // (Assuming you add an errorOccurred signal to TranscribeAudio)
    QObject::connect(this->m_worker,
                     &AudioAnnotationSegmentationWorker::errorOccurred,
                     this,
                     &AudioAnnotationSegmentationController::handleWorkerError);

    // 5. Lifecycle Management. Safe Start Lambda (Added 'this' context)
    auto worker = this->m_worker;
    QString inputJsonFile = info.url.path();

    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this->m_worker,
                     [worker, inputJsonFile]() {
                         if (worker) {
                             worker->segmentJsonContent(inputJsonFile);
                         }
                     }, Qt::QueuedConnection);


    // 7. Thread Stopped -> UI Notification (Data extraction happens here)
    QObject::connect(this->m_worker,
                     &AudioAnnotationSegmentationWorker::finished,
                     this,
                     &AudioAnnotationSegmentationController::handleFinished);

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

    errMsg = QString("Initializing Audio Annotation Segmentation...");

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioAnnotationSegmentationController::handleInput(...) --- " + errMsg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged(errMsg);
}

void AudioAnnotationSegmentationController::handleFinished(qint32 exitCode)
{
    QString msg;

    if (exitCode < 0)
    {
        msg = QString("Transcription segmentation failed.");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioAnnotationSegmentationController::handleFinished() --- " + msg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit statusChanged(msg);

        return;
    } else {

        msg = QString("Transcription segmentation complete.");

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioAnnotationSegmentationController::handleFinished() --- " + msg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(msg);

        FileInfo fInfo(this->m_worker->annotationManifest().toString(), FileType_t::AnnotationManifest);

        // Alert annotation file is ready!
        emit manifestReady(fInfo);
    }

    if (this->m_workerThread) this->m_workerThread->quit();
}

void AudioAnnotationSegmentationController::handleWorkerError(QString msg)
{
    QString errMsg = QString("Audio annotation segmentation failed: ").arg(msg);

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioAnnotationSegmentationController::handleWorkerError(...) --- " + errMsg,
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
