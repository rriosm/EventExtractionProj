
#include <QFileInfo>

#include <cstdio> // For fprintf, stdout, fflush

#include "LogUtils.h"

#include "TranscodeController.h"
#include "TranscodeWorker.h"

TranscodeController::TranscodeController(PipelineWorker *parent) :
    PipelineWorker(parent),
    m_workerThread(nullptr),
    m_worker(nullptr)
{
    this->m_status.clear();
    this->m_inMediaUrl.clear();
    this->m_outMediaUrl.clear();
}

TranscodeController::~TranscodeController()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
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

bool TranscodeController::operator==(const TranscodeController &right) const
{
    if ( (this->m_inMediaUrl != right.m_inMediaUrl) ||
         (this->m_outMediaUrl != right.m_outMediaUrl) )
    {
        return false;
    }

    return true;
}

bool TranscodeController::operator!=(const TranscodeController &right) const
{
    return !(*this == right);
}

bool TranscodeController::operator==(const TranscodeController *ptr) const
{
    return (this == ptr);
}

bool TranscodeController::operator!=(const TranscodeController *ptr) const
{
    return (this != ptr);
}

void TranscodeController::setupAudioTranscoding(const Url &newInMediaUrl, const Url & newOutMedia)
{
    if (newInMediaUrl != this->m_inMediaUrl)
        this->m_inMediaUrl = newInMediaUrl;

    if (newOutMedia != this->m_outMediaUrl)
        this->m_outMediaUrl = newOutMedia;
}

const Url & TranscodeController::inMediaUrl() const
{
    return this->m_inMediaUrl;
}

void TranscodeController::setInMediaUrl(const Url &newInMediaUrl)
{
    if (newInMediaUrl == this->m_inMediaUrl) return;

    this->m_inMediaUrl = newInMediaUrl;
}

const Url &TranscodeController::outMediaUrl() const
{
    return this->m_outMediaUrl;
}

void TranscodeController::setOutMediaUrl(const Url &newOutMediaUrl)
{
    if (this->m_outMediaUrl == newOutMediaUrl) return;

    this->m_outMediaUrl = newOutMediaUrl;
}

double TranscodeController::currentProgress() const
{
    return this->m_currentProgress;
}

void TranscodeController::cancelTranscoding()
{
    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        // Emitting this signal will trigger the worker's abort() slot
        // across the thread boundary safely.

        QString msg = QString("Aborting and cleaning up...");

#ifdef QT_DEBUG
        LogUtils::safeLog(QString("TranscodeController::cancelTranscoding() --- %1")
                          .arg(msg),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        this->setStatus(msg);

        emit statusChanged(msg);
        emit requestAbort();

        // Set connect to IVideoSegmentationWorker to alert controller to finish!
        // This sets a flag that QThread::currentThread()->isInterruptionRequested()
        // will return 'true' for inside the worker thread.
        this->m_workerThread->requestInterruption();

        // Note: We do NOT call quit() here. We wait for the curl callback
        // to see the flag, abort the transfer, and let the worker finish naturally.
    }
}

void TranscodeController::handleInput(FileInfo info)
{
    if (info.type == FileType_t::VideoFile)
    {
        if (info.url.isEmpty()) return;

        this->m_inMediaUrl = info.url;

        this->startTranscoding();
    }
}

 void TranscodeController::onWorkerProgress(double percent)
{
     // 1. Save the state safely
     m_currentProgress = percent;

     // 2. Draw the Console Progress Bar
     const int barWidth = 50;
     int pos = barWidth * (percent / 100.0);

     QString bar;
     for (int i = 0; i < barWidth; ++i) {
         if (i < pos) bar += "=";
         else if (i == pos) bar += ">";
         else bar += "-";
     }

     // Print with a carriage return (\r) to overwrite the current line in the terminal
     fprintf(stdout, "\r[Transcode] [%s] %3.0f%%  --- ", bar.toUtf8().constData(), percent);
     fflush(stdout); // Force the console to draw immediately

     // 3. Print a final newline when it hits 100% so the next pipeline stage logs cleanly below it
     if (percent >= 100.0)
     {
         fprintf(stdout, "\n");
         fflush(stdout);
     }

     // 4. Re-emit to the outside world (just in case main.cpp needs to know)
     emit progressUpdated(percent);
}

void TranscodeController::startTranscoding()
{
    QString errMsg;

    if (this->m_workerThread && this->m_workerThread->isRunning())
    {
        errMsg = QString("TranscodeController::startAudioTranscoding() --- A transcoding is already in progress.");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit statusChanged(errMsg);

        return;
    }

    QString inputFilePath = this->inMediaUrl().path();
    QString outputFilePath = this->outMediaUrl().path();

    if (inputFilePath.isEmpty())
    {
        errMsg = QString("TranscodeController::startAudioTranscoding() --- Video input file is empty!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);

        return;
    }

    QFileInfo inputFile(inputFilePath);

    if ( !inputFile.isFile() || !inputFile.exists())
    {
        errMsg = QString("TranscodeController::startAudioTranscoding() --- Video input file if invalid!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);

        return;
    }

    if (outputFilePath.isEmpty())
    {
        errMsg = QString("TranscodeController::startAudioTranscoding() --- Audio output file is empty!");

#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(errMsg);

        return;
    }

    this->m_workerThread = new QThread;
    this->m_worker = new TranscodeWorker(inputFilePath, outputFilePath);

    this->m_worker->moveToThread(this->m_workerThread);

    QObject::connect(this->m_worker,
                     &TranscodeWorker::progressUpdated,
                     this,
                     [this](double p){
        emit progressUpdated(static_cast<double>(p));
    });

    QObject::connect(this->m_worker,
                     &TranscodeWorker::error,
                     this,
                     &TranscodeController::handleWorkerError);

    QObject::connect(this->m_worker,
                     &TranscodeWorker::progressUpdated,
                     this,
                     &TranscodeController::onWorkerProgress);

    QObject::connect(this->m_workerThread,
                     &QThread::started,
                     this->m_worker,
                     &TranscodeWorker::process);

    QObject::connect(this->m_worker,
                     &TranscodeWorker::finished,
                     this,
                     &TranscodeController::handleFinished);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_worker,
                     &QObject::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QThread::finished,
                     this->m_workerThread,
                     &QThread::deleteLater);

    QObject::connect(this->m_workerThread,
                     &QObject::destroyed,
                     this,
                     [this](QObject *obj){
        if (this->m_workerThread == obj) {
            this->m_workerThread = nullptr;
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("TranscodeController::startAudioTranscoding() --- m_workerThread = nullptr"),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }
    });

    QObject::connect(this->m_worker,
                     &QObject::destroyed,
                     this,
                     [this](QObject *obj){
        if (this->m_worker == obj) {
            this->m_worker = nullptr;
#ifdef QT_DEBUG
            LogUtils::safeLog(QString("TranscodeController::startAudioTranscoding() --- m_worker = nullptr"),
                              __FILE__,
                              __LINE__,
                                       Q_FUNC_INFO);
#endif
        }
    });

    this->m_workerThread->start();

    errMsg = QString("TranscodeController::startAudioTranscoding() --- Initializing Transcoding...");

#ifdef QT_DEBUG
    LogUtils::safeLog(errMsg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit statusChanged("Initializing Transcoding...");
}

void TranscodeController::setStatus(const QString &newStatus)
{
    if (newStatus != this->m_status)
        this->m_status = newStatus;
}

void TranscodeController::handleFinished()
{
    QString msg = QString("Transcoding Complete.");

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("TranscodeController::handleFinished() --- %1")
                      .arg(msg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QString audioFilePath = this->m_outMediaUrl.path();

    FileInfo fInfo(this->m_outMediaUrl, FileType_t::AudioFile);

    emit finished(fInfo);

    emit statusChanged(msg);

    if (this->m_workerThread) this->m_workerThread->quit();
}

void TranscodeController::handleWorkerError(QString errorMsg)
{
    QString errMsg = QString("Transcription failed: %1").arg(errorMsg);

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("TranscodeController::handleWorkerError(...) --- %1")
                      .arg(errMsg),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // 1. Notify the UI via the public signal
    emit error(errorMsg);
    emit statusChanged(errMsg);

    // 2. Stop the thread
    if (this->m_workerThread)
    {
        this->m_workerThread->quit();
        this->m_workerThread->wait();
    }
}
