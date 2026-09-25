
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonArray>
// #include <QtConcurrent/QtConcurrent>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

#include "inc/AudioSegmentationWorker.h"
#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"

AudioSegmentationWorker::AudioSegmentationWorker(QObject *parent) :
    AudioSegmentationWorker(QString(), QString(), *Constants::Q_QGS_DEFAULT_CLIP_SEC, parent)
{
    // TODO
}

AudioSegmentationWorker::AudioSegmentationWorker(const QString &input,
                                                 const QString &dir,
                                                 qreal clipSec,
                                                 QObject *parent) :
    IAudioSegmentationWorker(parent),
    m_inputAudio(input),
    m_dir(dir),
    m_clipSec(clipSec),
    m_totalDurationSec(0.0),
    m_abort(false),
    m_retryCount(0)
{
    // TODO
}

AudioSegmentationWorker::~AudioSegmentationWorker()
{
    this->performAbort();
}

// --- Getters / Setters ---
// (Similar thread-safe implementations to your Video Worker omitted for brevity)
QString AudioSegmentationWorker::inputAudio() const
{
    QMutexLocker l(&this->m_mutexInputAudio);

    return this->m_inputAudio;
}

bool AudioSegmentationWorker::setInputAudio(const QString &newVal)
{
    QMutexLocker locker(&m_mutexInputAudio);

    if (newVal.isEmpty() || this->m_inputAudio == newVal) return false;

    this->m_inputAudio = newVal;

    return true;
}

QString AudioSegmentationWorker::outputDir() const
{
    QMutexLocker l(&this->m_mutexDir);

    return this->m_dir;
}

bool AudioSegmentationWorker::setOutputDir(const QString &newVal)
{
    QMutexLocker locker(&m_mutexDir);

    if (newVal.isEmpty() || this->m_dir == newVal) return false;

    this->m_dir = newVal;

    return true;
}

qreal AudioSegmentationWorker::clipSec() const
{
    QMutexLocker l(&this->m_mutexClipSec);

    return this->m_clipSec;
}

bool AudioSegmentationWorker::setClipSec(qreal newVal)
{
    QMutexLocker locker(&m_mutexClipSec);

    if (newVal < 0.00 || qFuzzyCompare(this->m_clipSec, newVal)) return false;

    this->m_clipSec = newVal;

    return true;
}

double AudioSegmentationWorker::totalDurationForUI() const
{
    QMutexLocker l(&this->m_mutexTotalDuration);

    return this->m_totalDurationSec;
}

void AudioSegmentationWorker::setTotalDurationForUI(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    AVFormatContext *pFormatCtx = nullptr;

    if (avformat_open_input(&pFormatCtx, filePath.toUtf8().constData(), nullptr, nullptr) >= 0)
    {
        if (avformat_find_stream_info(pFormatCtx, nullptr) >= 0)
        {
            QMutexLocker locker(&this->m_mutexTotalDuration);

            this->m_totalDurationSec = static_cast<double>(pFormatCtx->duration) / AV_TIME_BASE;
        }

        avformat_close_input(&pFormatCtx);
    }
}

double AudioSegmentationWorker::getDurationViaFFprobe(const QString &filePath)
{
    AVFormatContext* pFormatCtx = nullptr;

    // Open the audio file to read its container metadata
    if (avformat_open_input(&pFormatCtx, filePath.toUtf8().constData(), nullptr, nullptr) < 0)
    {
        return 0.0;
    }

    avformat_find_stream_info(pFormatCtx, nullptr);

    // Duration is in AV_TIME_BASE fractional seconds (typically 1,000,000)
    double duration = static_cast<double>(pFormatCtx->duration) / AV_TIME_BASE;

    avformat_close_input(&pFormatCtx);

    return duration;
}

// --- Status Tracking ---

QString AudioSegmentationWorker::processLastStatus() const
{
    QMutexLocker locker(&this->m_mutexStatus);

    return this->m_lastStatus;
}

bool AudioSegmentationWorker::setProcessLastStatus(const QString &lastStatus)
{
    QMutexLocker locker(&this->m_mutexStatus);

    if (lastStatus.isEmpty() || (this->m_lastStatus == lastStatus)) return false;

    this->m_lastStatus = lastStatus;

    return true;
}

// --- Main Processing Logic ---

void AudioSegmentationWorker::process()
{
    QString outDir = this->outputDir();
    QString inAudio = this->inputAudio();

    if (!ExtractorUtils::isDirWritable(outDir))
    {
        emit error(QString("Cannot write to output directory: %1. Check permissions.").arg(outDir));
        return;
    }

    {
        QMutexLocker locker(&this->m_mutexAbort);
        this->m_abort = false;
    }

    this->setTotalDurationForUI(inAudio);
    this->m_elapsedTimer.start();
    this->m_history.clear();

    // QtConcurrent::run([this]() {
    // });
    this->runFFmpegPipeline();
}

void AudioSegmentationWorker::stop()
{
    this->performAbort();
}

void AudioSegmentationWorker::abort()
{
    this->performAbort();
}

void AudioSegmentationWorker::performAbort()
{
    QMutexLocker locker(&this->m_mutexAbort);
    this->m_abort = true;
}

// --- CORE PIPELINE (Stream Copy) ---

void AudioSegmentationWorker::runFFmpegPipeline()
{
    // Restrict this worker thread to the first 4 cores
    ExtractorUtils::limit_to_cores(4);

    qint32 ret = 0;
    AVFormatContext *ifmt_ctx = nullptr;
    AVFormatContext *ofmt_ctx = nullptr;
    AVPacket *pkt = av_packet_alloc();

    try {
        emit statusUpdated("Opening Input...");

        // 1. Open Input
        if (avformat_open_input(&ifmt_ctx, this->inputAudio().toUtf8().constData(), nullptr, nullptr) < 0)
        {
            throw std::runtime_error("Could not open input audio file.");
        }

        if (avformat_find_stream_info(ifmt_ctx, nullptr) < 0)
        {
            throw std::runtime_error("Failed to find stream info.");
        }

        // 2. Find Audio Stream
        int audio_stream_idx = -1;

        for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++)
        {
            if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                audio_stream_idx = i;
                break;
            }
        }

        if (audio_stream_idx == -1) throw std::runtime_error("No audio stream found.");
        AVStream *in_stream = ifmt_ctx->streams[audio_stream_idx];

        emit statusUpdated("Configuring Segmentation...");

        // 3. Allocate Output Context with "segment" muxer
        QString outPattern = QDir(this->outputDir()).filePath("segment_%03d.mp3");

        if (avformat_alloc_output_context2(&ofmt_ctx, nullptr, "segment", outPattern.toUtf8().constData()) < 0)
        {
            throw std::runtime_error("Could not create output context.");
        }

        // 4. Create Output Stream and Copy Codec Parameters (NO RE-ENCODING)
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, nullptr);

        if (!out_stream) throw std::runtime_error("Failed allocating output stream.");

        if (avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar) < 0)
        {
            throw std::runtime_error("Failed to copy codec parameters.");
        }

        out_stream->codecpar->codec_tag = 0;

        // 5. Configure Segmenter Options
        qreal validClipSec = this->clipSec() > 0 ? this->clipSec() : *Constants::Q_QGS_DEFAULT_CLIP_SEC;
        av_opt_set(ofmt_ctx->priv_data, "segment_time", QString::number(validClipSec, 'f', 2).toUtf8().constData(), 0);
        av_opt_set(ofmt_ctx->priv_data, "segment_format", "mp3", 0);
        av_opt_set(ofmt_ctx->priv_data, "reset_timestamps", "1", 0);

        // 6. Open Output IO
        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        {
            if (avio_open(&ofmt_ctx->pb, outPattern.toUtf8().constData(), AVIO_FLAG_WRITE) < 0)
            {
                throw std::runtime_error("Could not open output file.");
            }
        }

        // 7. Write Header
        if (avformat_write_header(ofmt_ctx, nullptr) < 0)
        {
            throw std::runtime_error("Error occurred when opening output file.");
        }

        emit statusUpdated("Segmenting Audio...");

        // 8. Read and Write Packets
        int lastPercent = -1;
        double totalDuration = this->totalDurationForUI();

        while (av_read_frame(ifmt_ctx, pkt) >= 0)
        {
            // Check for abort
            {
                QMutexLocker l(&this->m_mutexAbort);

                if (this->m_abort)
                {
                    av_packet_unref(pkt);
                    throw std::runtime_error("Abort requested.");
                }
            }

            if (pkt->stream_index == audio_stream_idx)
            {
                // Calculate current time for progress bar
                double currentSeconds = pkt->pts * av_q2d(in_stream->time_base);

                if (totalDuration > 0)
                {
                    int p = static_cast<int>((currentSeconds / totalDuration) * 100.0);

                    if (p > lastPercent && p <= 100)
                    {
                        lastPercent = p;
                        emit progressUpdated(p);
                    }
                }

                this->handleProgressUpdate(currentSeconds);

                // Rescale timestamps for the output stream
                pkt->stream_index = out_stream->index;
                av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
                pkt->pos = -1; // Reset position so muxer recalculates it

                // Write packet
                if (av_interleaved_write_frame(ofmt_ctx, pkt) < 0)
                {
                    qWarning() << "Error writing packet to output";
                }
            }

            av_packet_unref(pkt);
        }

        // 9. Finalize
        emit statusUpdated("Finalizing...");
        av_write_trailer(ofmt_ctx);

    } catch (const std::exception &e) {
        ret = -1;
        emit error(QString("FFmpeg Error: %1").arg(e.what()));
    }

    // --- Cleanup ---
    av_packet_free(&pkt);

    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);

    if (ofmt_ctx)
    {
        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);

        avformat_free_context(ofmt_ctx);
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("AudioSegmentationWorker::runFFmpegPipeline() - Finishing: %1")
                      .arg(ret),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    this->onFinished(ret, 0);
}

void AudioSegmentationWorker::handleProgressUpdate(double currentSeconds)
{
    double total = this->totalDurationForUI();
    if (total <= 0.0) return;

    qint64 currentTimeMs = this->m_elapsedTimer.elapsed();
    this->m_history.enqueue({currentTimeMs, currentSeconds});

    if (m_history.size() > 20) this->m_history.dequeue();

    if (m_history.size() > 1)
    {
        auto first = this->m_history.head();
        auto last = this->m_history.last();

        double processedTimeSec = last.timeInSeconds - first.timeInSeconds;

        if (processedTimeSec > 0.001)
        {
            double msPerAudioSec = (double)(last.timestamp - first.timestamp) / processedTimeSec;
            double remainingAudioSec = total - currentSeconds;

            // Format ETA using your existing Utils function
            emit etaUpdated(ExtractorUtils::formatTime(static_cast<qint64>(remainingAudioSec * msPerAudioSec)));
        }
    }
}

void AudioSegmentationWorker::onFinished(qint32 exitCode, qint32 internalStatus)
{
#ifdef QT_DEBUG
    LogUtils::safeLog("AudioSegmentationWorker::onFinished() - Starting!",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    bool wasAborted = false;
    {
        QMutexLocker locker(&this->m_mutexAbort);
        wasAborted = this->m_abort;
    }

    if (exitCode != 0 || internalStatus != 0)
    {
        QString err = (exitCode != 0) ? LogUtils::ffmpegErrorToString(exitCode) : "Internal Error";

#ifdef QT_DEBUG
        LogUtils::safeLog("AudioSegmentationWorker::onFinished() - Finishing with error: " + err,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(err);

    } else
    if (wasAborted)
    {
        // Clean up partial audio segments
        ExtractorUtils::cleanupPartialAudioFiles(this->outputDir(), "segment_*.mp3");

        emit error(tr("Operation cancelled by user."));
    }

    this->m_retryCount = 0;

#ifdef QT_DEBUG
    LogUtils::safeLog("AudioSegmentationWorker::onFinished() - Finished!",
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit finished(exitCode, internalStatus);
}
