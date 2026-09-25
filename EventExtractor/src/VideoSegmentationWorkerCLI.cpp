
#include "VideoSegmentationWorkerCLI.h"

#include "ExtractorUtils.h"

#include "LogUtils.h"

#include <QDebug>
#include <QDir>
#include <QMap>
#include <QStringList>
#include <QProcess>
#include <QRegularExpression>
#include <QMutexLocker>
#include <QThread>

#include <QCoreApplication>
#include <QDateTime>

#include <QtGlobal>

// public

VideoSegmentationWorkerCLI::VideoSegmentationWorkerCLI(QObject *parent) :
    VideoSegmentationWorkerCLI(QString(), QString(), ModelCode_t::None, 0, *Constants::Q_QGS_DEFAULT_SLOWDOWN_FACTOR, parent)
{
    // Delagation to SegmentationWorker::SegmentationWorker(const QString, const QString, const qint32, qint32, QObject *)
}

VideoSegmentationWorkerCLI::VideoSegmentationWorkerCLI(const QString &input,
                                                       const QString &dir,
                                                       const ModelCode_t model,
                                                       qint32 clipSec,
                                                       qreal slowdownFactor,
                                                       QObject *parent) :
    IVideoSegmentationWorker(parent),
    m_inputVideo(input),
    m_dir(dir),
    m_modelCode(model),
    m_clipSec(clipSec > 0 ? clipSec : *Constants::Q_QGS_DEFAULT_CLIP_SEC),
    m_slowdownFactor(!qFuzzyIsNull(slowdownFactor) && slowdownFactor > 0 ? slowdownFactor : *Constants::Q_QGS_DEFAULT_SLOWDOWN_FACTOR),
    m_abort(false),
    m_process(new QProcess(this))
{
    qRegisterMetaType<VideoSegmentInfo>("VideoSegmentInfo");
    qRegisterMetaType<VideoSegmentInfo::VideoSegmentInfoList_t>("VideoSegmentInfo::VideoSegmentInfoList_t");

    // 2. Connect signals within the thread

    // Handle progress
    Q_ASSERT(connect(this->m_process,
                     &QProcess::readyReadStandardError,
                     this,
                     &VideoSegmentationWorkerCLI::readProgress,
                     Qt::UniqueConnection)
             );

    Q_ASSERT(connect(this->m_process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     this,
                     &VideoSegmentationWorkerCLI::onFinished)
             );

    Q_ASSERT(connect(this->m_process,
                     &QProcess::errorOccurred,
                     this,
                     &VideoSegmentationWorkerCLI::onFFmpegProcessError)
             );
}

VideoSegmentationWorkerCLI::~VideoSegmentationWorkerCLI()
{
    this->performAbort();
}

bool VideoSegmentationWorkerCLI::operator ==(const VideoSegmentationWorkerCLI & right) const
{
    if ( (this->m_inputVideo != right.m_inputVideo) ||
         (this->m_dir != right.m_dir) ||
         (this->m_modelCode != right.m_modelCode) ||
         (this->m_lastStatus != right.m_lastStatus) )
    {
        return (false);
    }

    // Safer check for qreal:
    // qFuzzyCompare is safe for non-zero values.
    // For values that might be zero, this is more robust:
    if (!qFuzzyCompare(this->m_clipSec + 1.00, right.m_clipSec + 1.00)) return false;

    return qFuzzyCompare(this->m_slowdownFactor + 1.00, right.m_slowdownFactor + 1.00);
}

bool VideoSegmentationWorkerCLI::operator !=(const VideoSegmentationWorkerCLI & right) const
{
    return !(*this == right);
}

bool VideoSegmentationWorkerCLI::operator==(const VideoSegmentationWorkerCLI *ptr) const
{
    return (this == ptr);
}

bool VideoSegmentationWorkerCLI::operator!=(const VideoSegmentationWorkerCLI *ptr) const
{
    return (this != ptr);
}

// private

QString VideoSegmentationWorkerCLI::inputVideo() const
{
    QMutexLocker locker(&m_mutexInputVideo);

    return (this->m_inputVideo);
}

bool VideoSegmentationWorkerCLI::setInputVideo(const QString & newInputVideo)
{
    QMutexLocker locker(&m_mutexInputVideo);

    if ( newInputVideo.isEmpty() ) return false;

    if (this->m_inputVideo != newInputVideo)
        this->m_inputVideo = newInputVideo;

    return true;
}

ModelCode_t VideoSegmentationWorkerCLI::modelCode() const
{
    QMutexLocker locker(&m_mutexModel);

    return (this->m_modelCode);
}

bool VideoSegmentationWorkerCLI::setModelCode(ModelCode_t newModel)
{
    QMutexLocker locker(&m_mutexModel);

    if (newModel == ModelCode_t::None )
        return false;

    if (newModel != this->m_modelCode)
        this->m_modelCode = newModel;

    return true;
}

QString VideoSegmentationWorkerCLI::dir() const
{
    QMutexLocker locker(&m_mutexDir);

    return (this->m_dir);
}

bool VideoSegmentationWorkerCLI::setDir(const QString &newDir)
{
    QMutexLocker locker(&m_mutexDir);

    if ( newDir.isEmpty() )
        return false;

    if (newDir != this->m_dir)
        this->m_dir = newDir;

    return true;
}

qreal VideoSegmentationWorkerCLI::clipSec() const
{
    QMutexLocker locker(&m_mutexClipSec);

    return (this->m_clipSec);
}

bool VideoSegmentationWorkerCLI::setClipSec(qreal newClipSec)
{
    QMutexLocker locker(&m_mutexClipSec);

    if (newClipSec <= 0  || qFuzzyCompare(newClipSec, this->m_clipSec)) return false;

    this->m_clipSec = newClipSec;

    return true;
}

qreal VideoSegmentationWorkerCLI::slowdownFactor() const
{
    QMutexLocker locker(&m_mutexSlowdownFactor);

    return this->m_slowdownFactor;
}

bool VideoSegmentationWorkerCLI::setSlowdownFactor(qreal newSlowdownFactor)
{
    QMutexLocker locker(&m_mutexSlowdownFactor);

    if (newSlowdownFactor <= 0 || qFuzzyCompare(this->m_slowdownFactor, newSlowdownFactor)) return false;

    this->m_slowdownFactor = newSlowdownFactor;

    return true;
}

qint32 VideoSegmentationWorkerCLI::totalFramesForUI() const
{
    QMutexLocker locker(&this->m_mutexTotalFrames);

    return (this->m_totalFrames);
}

void VideoSegmentationWorkerCLI::setTotalFramesForUI(const QString & filePath)
{
    QMutexLocker locker(&this->m_mutexTotalFrames);

    QProcess ffprobe;
    QStringList args = this->prepareTotalFrameArgsCmd(filePath);

    if (args.isEmpty())
    {
        emit error("Total frames command args setup failed!");

        qCritical() << "Segmentation command args setup failed!";

        return;
    }

    ffprobe.start("ffprobe", args);

    if ( ffprobe.waitForFinished(3000) )
    {
        bool ok;

        qint32 val = ffprobe.readAllStandardOutput().trimmed().toInt(&ok);

        this->m_totalFrames = ok ? val : 0;

    } else {
        this->m_totalFrames = 0;
    }
}

QString VideoSegmentationWorkerCLI::processLastStatus() const
{
    QMutexLocker locker(&m_mutexStatus);

    return (this->m_lastStatus);
}

bool VideoSegmentationWorkerCLI::setProcessLastStatus(const QString &lastStatus)
{
    QMutexLocker locker(&m_mutexStatus);

    if ( lastStatus.isEmpty() ||
        (lastStatus == m_lastStatus) )
    {
        return false;
    }

    this->m_lastStatus = lastStatus;

    return true;
}

void VideoSegmentationWorkerCLI::process()
{
    QString outDir = this->dir();
    QString inVideo = this->inputVideo();

    // 1. Validate Directory early
    if ( ! ExtractorUtils::isDirWritable(outDir) )
    {
        emit error(QString("Cannot write to output directory: %1. Check permissions.").arg(outDir));
        return;
    }

    // 2. Check Disk Space (Specific to macOS/Unix via QStorageInfo)
    if ( ! ExtractorUtils::hasSufficientDiskSpace(outDir, inVideo) )
    {
        emit error("VideoSegmentationWorkerCLI::process() --- Insufficient disk space to process this video.");
        return;
    }

    // 3. Prepare Arguments (using your logic from VideoProcessorData)
    // Note: For simplicity, assume getArgs is a helper or logic is inlined here
    QStringList segCmdArgs = this->prepareSegmentationCmdArgs();

    if ( segCmdArgs.isEmpty() )
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("CRITICAL: Segmentation arguments are empty!",
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error("Segmentation command args setup failed!");

        return;
    }
#ifdef QT_DEBUG
    LogUtils::safeLog("FFmpeg command prepared. Launching process...",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    if (this->m_process->state() != QProcess::NotRunning)
    {
        this->setProcessLastStatus(ExtractorUtils::ffmpegProcessStateToString(this->m_process->state()));

        emit error("A processing task is already in progress.");

        return;
    }

    // 4. Reset Abort flag and Start
    {
        QMutexLocker locker(&this->m_mutexAbort);
        this->m_abort = false;
    }

    this->m_history.clear();
    ExtractorUtils::cleanupVideoPartialFiles(this->dir(), "segment%1_*.mp4", this->modelCode());
    this->setTotalFramesForUI(this->inputVideo()); // Store this as a member variable
    this->m_elapsedTimer.start(); // Start timing right here

#ifdef QT_DEBUG
    LogUtils::safeLog("VideoSegmentationWorkerCLI::process() --- Starting process...",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // Start
    this->m_process->start("ffmpeg", segCmdArgs);

    if ( ! this->m_process->waitForStarted() )
    {
        emit error("Failed to start FFmpeg");
        emit finished(0, 0);

        return;
    }
}

void VideoSegmentationWorkerCLI::abort()
{
    this->performAbort();
}

void VideoSegmentationWorkerCLI::stop()
{
    this->performAbort();
}

double VideoSegmentationWorkerCLI::getDurationViaFFprobe(const QString &filePath)
{
    if ( ! QFile::exists(filePath) )
    {
        qWarning() << "getDurationViaFFprobe --- File does not exist:" << filePath;

        return 0.0;
    }

    QProcess ffprobe;
    QStringList args;

    args << "-v" << "error"
         << "-show_entries" << "format=duration"
         << "-of" << "default=noprint_wrappers=1:nokey=1"
         << filePath;

    ffprobe.start("ffprobe", args);

    // Wait for ffprobe to finish (usually takes milliseconds)
    if ( ! ffprobe.waitForFinished(3000)) // 3-second timeout
    {
        qWarning() << "getDurationViaFFprobe --- Timeout or process error:" << ffprobe.errorString();
        return 0.0;
    }

    // Read the output and convert to double
    QByteArray output = ffprobe.readAllStandardOutput().trimmed();

    if (output.isEmpty())
    {
        qWarning() << "getDurationViaFFprobe --- No output from ffprobe for:" << filePath;
        return 0.0;
    }

    bool ok;
    double duration = output.toDouble(&ok);

    if ( ! ok )
    {
        qWarning() << "getDurationViaFFprobe --- Failed to parse duration string:" << output;
        return 0.0;
    }

    return (duration);
}

QStringList VideoSegmentationWorkerCLI::prepareSegmentationCmdArgs()
{
    QString inVideo = this->inputVideo();
    QString seg_dir = this->dir();
    ModelCode_t mdlCode = this->modelCode();
    qreal clpSec = this->clipSec();

    if ( inVideo.isEmpty() ||
         seg_dir.isEmpty() )
    {
        return {};
    }

    // 1. Validation & Setup
    if ( ! QFile::exists(this->inputVideo()) )
    {
        qCritical() << "Input file does not exist:" << this->inputVideo();
        return {};
    }

    // 2. Check and create target dir
    QDir dir;
    if ( ! dir.exists(this->dir()) )
    {
        if ( ! dir.mkpath(this->dir()) )
        {
            return {};
        }
    }

    // 3. Get config model
    VideoModelConfig cfg = ExtractorUtils::getConfig(mdlCode);

    if ( ! cfg.isValid() )
    {
        qCritical() << "Unsupported model code:" << static_cast<int>(mdlCode);
        return {};
    }

    QString outputPattern = seg_dir + "/segment" + cfg.suffix() + "_%03d.mp4";

    int thrdsNumber = ExtractorUtils::optimalThreadCount();

    if (thrdsNumber > 0)
    {
#ifdef QT_DEBUG
        LogUtils::safeLog(QString("Optimizing FFmpeg with %1 threads").arg(thrdsNumber),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
    }

    // 4. Build QProcess Arguments (Equivalent to the ffmpeg-python pipeline)
    QStringList args;
    args << "-y"
         << "-i" << inVideo
         << "-threads" << QString::number(thrdsNumber)
         << "-f" << "lavfi" << "-i" << "anullsrc=channel_layout=stereo:sample_rate=44100"
         << "-filter_complex" << QString("[0:v]fps=%1,setpts=%2*PTS[v]").arg(cfg.fps()).arg(this->slowdownFactor(), 0, 'f', 2)
         << "-map" << "[v]"
         << "-map" << "1:a:0";

    // 3. Encoder Specific Logic
    QString encoder = ExtractorUtils::bestVideoEncoder();
    args << "-c:v" << encoder;

    if (encoder == "h264_videotoolbox") {
        // Hardware specific settings
        // -b:v 6000k provides high quality similar to CRF 23
        args << "-b:v" << "6000k"
             << "-pix_fmt" << "nv12";
    } else {
        // libx264 CPU specific settings
        args << "-crf" << QString::number(cfg.crf())
             << "-preset" << "slow"
             << "-pix_fmt" << "yuv420p";
    }

    qreal safeClipSec = (this->clipSec() > 0.00) ? clpSec : *Constants::Q_QGS_DEFAULT_CLIP_SEC;

    args << "-c:a" << "aac"
         << "-shortest"
         << "-f" << "segment"
         << "-segment_time" << QString::number(safeClipSec, 'f', 2)
         << "-reset_timestamps" << "1"
         << outputPattern;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Optimizing FFmpeg with %1").arg(args.join(" ")),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    return (args);
}

QStringList VideoSegmentationWorkerCLI::prepareTotalFrameArgsCmd(const QString & filePath)
{
    if (filePath.isEmpty())
    {
        return {};
    }

    QStringList args;

    args << "-v" << "error" << "-select_streams" << "v:0"
         << "-count_packets" << "-show_entries" << "stream=nb_read_packets"
         << "-of" << "csv=p=0" << filePath;

    return args;
}

void VideoSegmentationWorkerCLI::performAbort()
{
    QMutexLocker locker(&this->m_mutexAbort);

    // 1. Set the flag so onFinished() knows this was intentional
    this->m_abort = true;

    if (this->m_process && this->m_process->state() != QProcess::NotRunning)
    {
        // 2. Try to terminate gracefully first (sends SIGTERM)
        // This allows FFmpeg to close file handles properly
        this->m_process->terminate();

        // 3. Give it a small window to close nicely
        if (!this->m_process->waitForFinished(2000))
        {
            // 4. If it's still running after 2 seconds, kill it (sends SIGKILL)
            this->m_process->kill();
            this->m_process->waitForFinished(1000);
        }
    }

    // 5. Clear the smoothing history so it's fresh for next time
    this->m_history.clear();
}

void VideoSegmentationWorkerCLI::readProgress()
{
    QMutexLocker locker(&this->m_mutexAbort);

    if ( this->m_abort ||
         ! this->m_process) return;

    QString output = QString::fromLocal8Bit(this->m_process->readAllStandardError());

    // 3. Robust Parsing
    // FFmpeg output can be "frame=  123 fps=..." or "frame=1024..."
    // We use a regex that handles whitespace and matches the LAST occurrence in the buffer
    static QRegularExpression re("frame=\\s*(\\d+)");
    QRegularExpressionMatchIterator i = re.globalMatch(output);
    QString lastFrameMatch;

    while (i.hasNext())
    {
        QRegularExpressionMatch match = i.next();
        lastFrameMatch = match.captured(1);
    }

    // 4. Update progress if a match was found
    if ( ! lastFrameMatch.isEmpty() )
    {
        qint32 currentFrame = lastFrameMatch.toInt();
        qint32 total = this->totalFramesForUI();
        qint64 currentTime = m_elapsedTimer.elapsed();

        if (total > 0)
        {
            // 1. Calculate Progress %
            // Use floating point for precision before casting back to int
            int progress = static_cast<int>((static_cast<double>(currentFrame) / total) * 100);

            // Ensure we don't exceed 100% due to slight metadata discrepancies
            progress = qBound(0, progress, 100);

            emit progressUpdated(progress);

            // 2. Manage Moving Average Buffer
            this->m_history.enqueue({currentTime, currentFrame});

            if (this->m_history.size() > m_historyLimit)
            {
                this->m_history.dequeue();
            }

            // 3. Calculate Smooth Velocity
            if (m_history.size() > 1)
            {
                const auto &first = m_history.head();
                const auto &last = m_history.last();

                qint64 timeDelta = last.timestamp - first.timestamp;
                qint32 frameDelta = last.frame - first.frame;

                if (timeDelta > 0 && frameDelta > 0)
                {
                    double msPerFrame = static_cast<double>(timeDelta) / frameDelta;
                    qint32 framesRemaining = total - currentFrame;
                    qint64 etaMs = static_cast<qint64>(msPerFrame * framesRemaining);

                    emit etaUpdated(ExtractorUtils::formatTime(etaMs));
                }
            }
        }
    }
}

void VideoSegmentationWorkerCLI::onFinished(qint32 exitCode, QProcess::ExitStatus status)
{
    bool wasAborted = false;

    {
        QMutexLocker locker(&this->m_mutexAbort);
        wasAborted = this->m_abort;
    }

    if (status == QProcess::CrashExit)
    {
        emit error("FFmpeg crashed.");

    } else if (exitCode != 0)
    {
        emit error(QString("FFmpeg error code: %1").arg(exitCode));

    } else
    if (wasAborted)
    {
        ExtractorUtils::cleanupVideoPartialFiles(this->dir(), "segment%1_*.mp4", this->modelCode());

        emit error("Operation cancelled.");

    }

    // Reset internal status tracking
    this->setProcessLastStatus("Finished");

    emit finished(exitCode, status); // Signal to wrap up the thread
}

void VideoSegmentationWorkerCLI::onFFmpegProcessError([[maybe_unused]] QProcess::ProcessError pError)
{
    QString userMessage;

    switch (pError) {
        case QProcess::FailedToStart:
        {
            userMessage = "FFmpeg not found. Please ensure it is installed and in your system PATH.";

            break;
        }

        case QProcess::Crashed:
        {
            userMessage = "FFmpeg crashed during processing. The video file might be corrupt.";

            break;
        }

        case QProcess::Timedout:
        {
            userMessage = "The operation timed out.";

            break;
        }

        case QProcess::WriteError:
        {
            userMessage = "Write error.";
            break;
        }

        case QProcess::ReadError:
        {
            userMessage = "Read error.";
            break;
        }

        case QProcess::UnknownError:
        {
            userMessage = "Unknown error.";
            break;
        }

        default:
        {
            userMessage = "An unexpected error occurred: " + this->m_process->errorString();
            break;
        }
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Process Error (%1): %2").arg(pError).arg(userMessage),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    QMutexLocker locker(&this->m_mutexAbort);

    if ( ! this->m_abort )
    {
        emit error(userMessage);
    }
}
