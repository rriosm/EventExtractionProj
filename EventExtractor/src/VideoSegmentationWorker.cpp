
#include <QDir>
#include <QElapsedTimer>
#include <QMutexLocker>
#include <QDateTime>
#include <QFileInfo>
#include <QTimer>
// #include <QtConcurrent/QtConcurrent>

#include <cmath>

#include "inc/ffmpeg_gen.h"
#include "inc/VideoSegmentationWorker.h"
#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"

VideoSegmentationWorker::VideoSegmentationWorker(QObject *parent) :
    VideoSegmentationWorker(QString(), QString(), ModelCode_t::None, 0, *Constants::Q_QGS_DEFAULT_SLOWDOWN_FACTOR, parent)
{
    // TODO
}

VideoSegmentationWorker::VideoSegmentationWorker(const QString &input,
                                       const QString &dir,
                                       const ModelCode_t model,
                                       qint32 clipSec,
                                       qreal slowdownFactor,
                                       QObject *parent) :
    IVideoSegmentationWorker(parent),
    m_inputVideo(input),
    m_dir(dir),
    m_modelCode(model),
    m_clipSec(clipSec > 0.00 ? clipSec : *Constants::Q_QGS_DEFAULT_CLIP_SEC),
    m_slowdownFactor(!qFuzzyIsNull(slowdownFactor) && slowdownFactor > 0 ? slowdownFactor : *Constants::Q_QGS_DEFAULT_SLOWDOWN_FACTOR),
    m_totalFrames(0),
    m_abort(false),
    m_retryCount(0)
{
    // qRegisterMetaType<VideoSegmentInfo>("VideoSegmentInfo");
    // qRegisterMetaType<VideoSegmentInfo::VideoSegmentInfoList_t>("VideoSegmentInfo::VideoSegmentInfoList_t");
}

VideoSegmentationWorker::~VideoSegmentationWorker()
{
    this->performAbort();
    ExtractorUtils::cleanupVideoPartialFiles(m_dir, "segment%1_*.mp4", this->m_modelCode);
}

// --- Getters / Setters ---

bool VideoSegmentationWorker::operator ==(const VideoSegmentationWorker & right) const
{
    if ( (this->m_inputVideo != right.m_inputVideo) ||
         (this->m_dir != right.m_dir) ||
         (this->m_modelCode != right.m_modelCode) ||
         (!qFuzzyCompare(this->m_clipSec + 1.00, right.m_clipSec + 1.00)) ||
         (this->m_lastStatus != right.m_lastStatus) )
    {
        return false;
    }

    // Safer check for qreal:
    // qFuzzyCompare is safe for non-zero values.
    // For values that might be zero, this is more robust:
    if (!qFuzzyCompare(this->m_clipSec + 1.00, right.m_clipSec + 1.00)) return false;

    return qFuzzyCompare(this->m_slowdownFactor + 1.0, right.m_slowdownFactor + 1.0);
}

bool VideoSegmentationWorker::operator !=(const VideoSegmentationWorker & right) const
{
    return !(*this == right);
}

bool VideoSegmentationWorker::operator==(const VideoSegmentationWorker *ptr) const
{
    return (this == ptr);
}

bool VideoSegmentationWorker::operator!=(const VideoSegmentationWorker *ptr) const
{
    return (this != ptr);
}

QString VideoSegmentationWorker::inputVideo() const
{
    QMutexLocker locker(&m_mutexInputVideo);

    return this->m_inputVideo;
}

bool VideoSegmentationWorker::setInputVideo(const QString & newVal)
{
    QMutexLocker locker(&m_mutexInputVideo);

    if (newVal.isEmpty() || this->m_inputVideo == newVal) return false;

    this->m_inputVideo = newVal;

    return true;
}

ModelCode_t VideoSegmentationWorker::modelCode() const
{
    QMutexLocker locker(&m_mutexModel);

    return this->m_modelCode;
}

bool VideoSegmentationWorker::setModelCode(ModelCode_t newVal)
{
    QMutexLocker locker(&m_mutexModel);

    if (newVal == ModelCode_t::None || this->m_modelCode == newVal) return false;

   this->m_modelCode = newVal;

    return true;
}

QString VideoSegmentationWorker::dir() const
{
    QMutexLocker locker(&m_mutexDir);

    return this->m_dir;
}

bool VideoSegmentationWorker::setDir(const QString &newVal)
{
    QMutexLocker locker(&m_mutexDir);

    if (newVal.isEmpty() || this->m_dir == newVal) return false;

    this->m_dir = newVal;

    return true;
}

qreal VideoSegmentationWorker::clipSec() const
{
    QMutexLocker locker(&m_mutexClipSec);

    return this->m_clipSec;
}

bool VideoSegmentationWorker::setClipSec(qreal newVal)
{
    QMutexLocker locker(&m_mutexClipSec);

    if (newVal <= 0 || this->m_clipSec == newVal) return false;

    this->m_clipSec = newVal;

    return true;
}

qreal VideoSegmentationWorker::slowdownFactor() const
{
    QMutexLocker locker(&m_mutexSlowdownFactor);

    return this->m_slowdownFactor;
}

bool VideoSegmentationWorker::setSlowdownFactor(qreal newSlowdownFactor)
{
    QMutexLocker locker(&m_mutexSlowdownFactor);

    if (newSlowdownFactor < 0.00 || qFuzzyCompare(this->m_slowdownFactor, newSlowdownFactor)) return false;

    this->m_slowdownFactor = newSlowdownFactor;

    return true;
}

qint32 VideoSegmentationWorker::totalFramesForUI() const
{
    QMutexLocker locker(&m_mutexTotalFrames);

    return this->m_totalFrames;
}
void VideoSegmentationWorker::setTotalFramesForUI(const QString &filePath)
{
    if (filePath.isEmpty()) return;

    AVFormatContext *pFormatCtx = nullptr;

    if (avformat_open_input(&pFormatCtx, filePath.toUtf8().constData(), nullptr, nullptr) >= 0)
    {
        if (avformat_find_stream_info(pFormatCtx, nullptr) >= 0 )
        {
            int videoIdx = -1;

            for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
            {
                if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { videoIdx = i; break; }
            }

            qint64 count = this->getFrameCount(pFormatCtx, videoIdx);

            QMutexLocker locker(&m_mutexTotalFrames);

            this->m_totalFrames = count;
        }

        avformat_close_input(&pFormatCtx);
    }
}

double VideoSegmentationWorker::getDurationViaFFprobe(const QString &filePath)
{
    AVFormatContext* pFormatCtx = nullptr;

    if (avformat_open_input(&pFormatCtx, filePath.toUtf8().constData(), nullptr, nullptr) < 0) return 0.0;

    avformat_find_stream_info(pFormatCtx, nullptr);
    double duration = static_cast<double>(pFormatCtx->duration) / AV_TIME_BASE;
    avformat_close_input(&pFormatCtx);

    return duration;
}

QString VideoSegmentationWorker::processLastStatus() const
{
    QMutexLocker locker(&m_mutexStatus);
    return this->m_lastStatus;
}

bool VideoSegmentationWorker::setProcessLastStatus(const QString &lastStatus)
{
    QMutexLocker locker(&m_mutexStatus);

    if ( lastStatus.isEmpty() ||
         (this->m_lastStatus == lastStatus) )
        return false;

    this->m_lastStatus = lastStatus;

    return true;
}

// --- Main Processing Logic ---

void VideoSegmentationWorker::process()
{
    QString outDir = this->dir();
    QString inVideo = this->inputVideo();

    if ( ! ExtractorUtils::isDirWritable(outDir) )
    {
        emit error(QString("Cannot write to output directory: %1. Check permissions.").arg(outDir));
        return;
    }

    if ( ! ExtractorUtils::hasSufficientDiskSpace(outDir, inVideo) )
    {
        emit error("VideoSegmentationWorker::process() --- Insufficient disk space to process this video.");
        return;
    }

    {
        QMutexLocker locker(&this->m_mutexAbort);
        this->m_abort = false;
    }

    LogUtils::logFFmpegDiagnostics();
    ExtractorUtils::cleanupVideoPartialFiles(this->dir(), "segment%1_*.mp4", this->modelCode());
    this->setTotalFramesForUI(inVideo);
    this->m_elapsedTimer.start();

    // QtConcurrent::run([this]() {
    // });
    this->runFFmpegPipeline();
}

void VideoSegmentationWorker::stop()
{
    this->performAbort();
}

void VideoSegmentationWorker::abort()
{
    this->performAbort();
}

qint64 VideoSegmentationWorker::calculateTotalFrames(AVFormatContext *fmt_ctx, int videoStreamIndex)
{
    return this->getFrameCount(fmt_ctx, videoStreamIndex);
}

qint64 VideoSegmentationWorker::getFrameCount(AVFormatContext *pFormatCtx, int videoStreamIdx)
{
    if (!pFormatCtx || videoStreamIdx < 0) return 0;

    AVStream *vStream = pFormatCtx->streams[videoStreamIdx];

    qint64 frames = vStream->nb_frames;

    if (frames <= 0)
    {
        double durationSec = 0.0;

        if (vStream->duration != AV_NOPTS_VALUE)
        {
            durationSec = vStream->duration * av_q2d(vStream->time_base);
        } else if (pFormatCtx->duration != AV_NOPTS_VALUE) {
            durationSec = (double)pFormatCtx->duration / AV_TIME_BASE;
        }

        double fps = av_q2d(vStream->avg_frame_rate);
        if (fps <= 0) fps = av_q2d(vStream->r_frame_rate);

        if (durationSec > 0 && fps > 0) frames = static_cast<qint64>(qRound(durationSec * fps));
    }

    return frames;
}

// --- CORE PIPELINE ---

void VideoSegmentationWorker::initFilterGraph(AVFilterGraph **graph,
                                              AVFilterContext **src,
                                              AVFilterContext **sink,
                                              AVFrame *frame,
                                              AVRational inputTimeBase,
                                              double fps,
                                              AVPixelFormat targetPixFmt)
{
    *graph = avfilter_graph_alloc();
    if (!*graph) throw std::runtime_error("Failed to allocate filter graph");

    int width = frame->width;
    int height = frame->height;
    int fmt = frame->format;
    int sar_num = frame->sample_aspect_ratio.num ? frame->sample_aspect_ratio.num : 1;
    int sar_den = frame->sample_aspect_ratio.den ? frame->sample_aspect_ratio.den : 1;
    int tb_num = (inputTimeBase.num > 0) ? inputTimeBase.num : 1;
    int tb_den = (inputTimeBase.den > 0) ? inputTimeBase.den : 1000;

    const char *pix_fmt_name = av_get_pix_fmt_name((AVPixelFormat)fmt);

    QString args = QString("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
                       .arg(width)
                       .arg(height)
                       .arg(pix_fmt_name)
                       .arg(tb_num)
                       .arg(tb_den)
                       .arg(sar_num)
                       .arg(sar_den);

    int ret = avfilter_graph_create_filter(src, avfilter_get_by_name("buffer"), "in", args.toUtf8().constData(), nullptr, *graph);
    if (ret < 0) throw std::runtime_error("Filter input creation failed");

    AVBufferSrcParameters *par = av_buffersrc_parameters_alloc();

    if (par)
    {
        par->width = width;
        par->height = height;
        par->format = fmt;
        par->time_base = inputTimeBase;
        par->sample_aspect_ratio = frame->sample_aspect_ratio;
        par->color_space = frame->colorspace;
        par->color_range = frame->color_range;
        av_buffersrc_parameters_set(*src, par);
        av_free(par);
    }

    ret = avfilter_graph_create_filter(sink, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, *graph);
    if (ret < 0) throw std::runtime_error("Filter output creation failed");

    enum AVPixelFormat pix_fmts[] = { targetPixFmt, AV_PIX_FMT_NONE };
    av_opt_set_int_list(*sink,
                        "pix_fmts",
                        pix_fmts,
                        AV_PIX_FMT_NONE,
                        AV_OPT_SEARCH_CHILDREN);

    // Filter: FPS
    // We use QString::number with 'f' format to ensure double precision (e.g. 23.9760) is preserved as string
    AVFilterContext *fps_ctx;
    QString fpsArgs = QString::number(fps, 'f', 4);
    ret = avfilter_graph_create_filter(&fps_ctx, avfilter_get_by_name("fps"), "fps", fpsArgs.toUtf8().constData(), nullptr, *graph);
    if (ret < 0) throw std::runtime_error("Filter FPS creation failed");

    // AVFilterContext *setpts_ctx = nullptr;

    // if (this->slowdownFactor() > 0.0)
    // {
    //     // Filter: SetPTS (Slow down by factor X). Best for soccer matches
    //     QString slowDownFactorArg = QString("%1*PTS").arg(this->slowdownFactor(), 0, 'f', 2);

    //     ret = avfilter_graph_create_filter(&setpts_ctx, avfilter_get_by_name("setpts"),
    //                                        "setpts",
    //                                        slowDownFactorArg.toUtf8().constData(),
    //                                        nullptr,
    //                                        *graph);
    // }

    // if (ret < 0) throw std::runtime_error("Filter SETPTS creation failed");

    // Filter: Scale. Force dimensions to be divisible by 2 to prevent stride/black frame issues
    AVFilterContext *scale_ctx;
    ret = avfilter_graph_create_filter(&scale_ctx, avfilter_get_by_name("scale"), "scale", "w=trunc(iw/2)*2:h=trunc(ih/2)*2", nullptr, *graph);
    if (ret < 0) throw std::runtime_error("Filter SCALE creation failed");

    // Filter: Format (Pixel Format Conversion)
    const char *target_fmt_name = av_get_pix_fmt_name(targetPixFmt);
    QString format_args = QString("pix_fmts=%1").arg(target_fmt_name);

    AVFilterContext *format_ctx;
    ret = avfilter_graph_create_filter(&format_ctx, avfilter_get_by_name("format"), "format", format_args.toUtf8().constData(), nullptr, *graph);
    if (ret < 0) throw std::runtime_error("Filter FORMAT creation failed");

    // 5. Link Filters (Strict Linear Chain)
    // IMPORTANT: Ensure no other 'avfilter_link' calls exist outside this block!

    // 5. Link Filters (Strict Linear Chain)
    // IMPORTANT: Ensure no other 'avfilter_link' calls exist outside this block!
    // if (avfilter_link(*src, 0, fps_ctx, 0) < 0) throw std::runtime_error("Link src->fps failed");

    // if (this->slowdownFactor() > 0.0)
    // {
    //     if (avfilter_link(fps_ctx, 0, setpts_ctx, 0) < 0) throw std::runtime_error("Link fps->setpts failed");
    //     if (avfilter_link(setpts_ctx, 0, scale_ctx, 0) < 0) throw std::runtime_error("Link setpts->scale failed");
    // } else {
    //     if (avfilter_link(fps_ctx, 0, scale_ctx, 0) < 0) throw std::runtime_error("Link setpts->scale failed");
    // }


    // if ( ! setpts_ctx ) throw std::runtime_error("Invalid pts");

    if (this->slowdownFactor() > 0.0)
    {
        AVFilterContext *setpts_ctx = nullptr;

        // Filter: SetPTS (Slow down by factor X). Best for soccer matches
        QString slowDownFactorArg = QString("(PTS-STARTPTS)*%1").arg(this->slowdownFactor(), 0, 'f', 2);

        ret = avfilter_graph_create_filter(&setpts_ctx, avfilter_get_by_name("setpts"),
                                           "setpts",
                                           slowDownFactorArg.toUtf8().constData(),
                                           nullptr,
                                           *graph);

        if (ret < 0) throw std::runtime_error("Filter SETPTS creation failed");

        if (avfilter_link(*src, 0, setpts_ctx, 0) < 0) throw std::runtime_error("Link src->setpts failed");
        if (avfilter_link(setpts_ctx, 0, fps_ctx, 0) < 0) throw std::runtime_error("Link setpts->fps failed");
    } else {
        if (avfilter_link(*src, 0, fps_ctx, 0) < 0) throw std::runtime_error("Link src->fps failed");
    }

    if (avfilter_link(fps_ctx, 0, scale_ctx, 0) < 0) throw std::runtime_error("Link fps->scale failed");
    if (avfilter_link(scale_ctx, 0, format_ctx, 0) < 0) throw std::runtime_error("Link scale->format failed");
    if (avfilter_link(format_ctx, 0, *sink, 0) < 0) throw std::runtime_error("Link format->sink failed");

    // 6. Configure Graph
    if (avfilter_graph_config(*graph, nullptr) < 0) throw std::runtime_error("Filter config failed");
}

void VideoSegmentationWorker::runFFmpegPipeline()
{
    // Restrict this worker thread to the first 4 cores
    ExtractorUtils::limit_to_cores(4);

    qint32 ret = 0;
    qint32 exitCode = 0;
    qint32 internalStatus = 0;
    QElapsedTimer timer;
    timer.start();

    qint64 processedFrames = 0;
    qint32 lastPercent = -1;

    AVFormatContext *ifmt_ctx = nullptr, *ofmt_ctx = nullptr;
    AVCodecContext *dec_ctx = nullptr, *v_enc_ctx = nullptr, *a_enc_ctx = nullptr;
    AVFilterGraph *filter_graph = nullptr;
    AVFilterContext *buffersrc_ctx = nullptr, *buffersink_ctx = nullptr;

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    AVFrame *audio_frame = av_frame_alloc();

    VideoModelConfig cfg = ExtractorUtils::getConfig(this->modelCode());
    const QString currentInputVideo = this->inputVideo();

    try {
        emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::OpeningInput));

        if (avformat_open_input(&ifmt_ctx, currentInputVideo.toUtf8().constData(), nullptr, nullptr) < 0) throw std::runtime_error("Could not open input file");

        if (avformat_find_stream_info(ifmt_ctx, nullptr) < 0) throw std::runtime_error("Failed to find stream info");

        qint32 video_stream_idx = -1;

        for (unsigned int i = 0; i < ifmt_ctx->nb_streams; i++)
        {
            if (ifmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) { video_stream_idx = i; break; }
        }

        if (video_stream_idx == -1) throw std::runtime_error("No video stream found");

        AVStream *in_stream = ifmt_ctx->streams[video_stream_idx];
        qint64 targetTotal = this->getFrameCount(ifmt_ctx, video_stream_idx);

        double pipelineFps = cfg.fps();

        if (pipelineFps <= 0.0001)
        {
            pipelineFps = av_q2d(in_stream->avg_frame_rate);
            if (pipelineFps <= 0.0001) pipelineFps = 30.0;
        }

        const AVCodec *decoder = avcodec_find_decoder(in_stream->codecpar->codec_id);
        dec_ctx = avcodec_alloc_context3(decoder);
        avcodec_parameters_to_context(dec_ctx, in_stream->codecpar);

        // 0 tells FFmpeg to autodetect (usually 1.5 * num_cores),
        // but you can hardcode this to any value (e.g., 4).
        dec_ctx->thread_count = 4;

        if (avcodec_open2(dec_ctx, decoder, nullptr) < 0) throw std::runtime_error("Failed to open decoder");

        emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::ConfiguringCodecs));

        const AVCodec *v_encoder = avcodec_find_encoder_by_name(ExtractorUtils::bestVideoEncoder().toUtf8().constData());

        if (!v_encoder) v_encoder = avcodec_find_encoder(AV_CODEC_ID_H264);

        v_enc_ctx = avcodec_alloc_context3(v_encoder);
        v_enc_ctx->height = dec_ctx->height;
        v_enc_ctx->width = dec_ctx->width;

        // 0 tells FFmpeg to autodetect (usually 1.5 * num_cores),
        // but you can hardcode this to any value (e.g., 4).
        v_enc_ctx->thread_count = 4;

        // [FIX] Use YUV420P. It is the most robust format for segmenting.
        // We will force alignment in the filter graph to ensure HW compatibility.
        v_enc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;

        AVRational target_framerate = av_d2q(pipelineFps, 100000);
        v_enc_ctx->framerate = target_framerate;
        v_enc_ctx->time_base = av_inv_q(target_framerate);

        if (v_encoder->id == AV_CODEC_ID_H264) v_enc_ctx->profile = FF_PROFILE_H264_MAIN;

        AVDictionary *v_opts = nullptr;

        if (ExtractorUtils::bestVideoEncoder().contains("videotoolbox"))
        {
            av_dict_set(&v_opts, "realtime", "1", 0);
            av_dict_set(&v_opts, "allow_sw", "1", 0);
            av_dict_set(&v_opts, "q", "65", 0);
        } else {
            av_dict_set(&v_opts, "preset", "fast", 0);
            av_dict_set(&v_opts, "crf", QString::number(cfg.crf()).toUtf8().constData(), 0);
        }

        if (avcodec_open2(v_enc_ctx, v_encoder, &v_opts) < 0)
        {
            av_dict_free(&v_opts);
            throw std::runtime_error("Could not open video encoder");
        }

        av_dict_free(&v_opts);

        const AVCodec *a_encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
        a_enc_ctx = avcodec_alloc_context3(a_encoder);
        a_enc_ctx->sample_fmt = AV_SAMPLE_FMT_FLTP;
        a_enc_ctx->sample_rate = 44100;
        a_enc_ctx->time_base = {1, 44100};
        a_enc_ctx->bit_rate = 128000;

#if FFMPEG_NEW_CHANNEL_LAYOUT
        av_channel_layout_default(&a_enc_ctx->ch_layout, 2);
#else
        a_enc_ctx->channel_layout = AV_CH_LAYOUT_STEREO;
        a_enc_ctx->channels = 2;
#endif

        if (avcodec_open2(a_enc_ctx, a_encoder, nullptr) < 0)
            throw std::runtime_error("Could not open audio encoder");

        QString outPattern = this->dir() + "/segment" + cfg.suffix() + "_%03d.mp4";
        avformat_alloc_output_context2(&ofmt_ctx, nullptr, "segment", outPattern.toUtf8().constData());

        AVStream *v_out = avformat_new_stream(ofmt_ctx, nullptr);
        avcodec_parameters_from_context(v_out->codecpar, v_enc_ctx);
        v_out->time_base = v_enc_ctx->time_base;

        AVStream *a_out = avformat_new_stream(ofmt_ctx, nullptr);
        avcodec_parameters_from_context(a_out->codecpar, a_enc_ctx);
        a_out->time_base = a_enc_ctx->time_base;

        // DISABLE Global Header for Segment Muxing on macOS
        // This ensures SPS/PPS info is written to each segment, fixing "missing picture" errors.
        if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        {
            // Explicitly DO NOT set this flag for VideoToolbox + Segmenter
            // v_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

            // Audio is usually fine, but safe to keep local too
            // a_enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        }

        qreal validClipSec = this->clipSec() > 0.00 ? this->clipSec() : 60.00;
        av_opt_set(ofmt_ctx->priv_data, "segment_time", QString::number(validClipSec, 'f', 2).toUtf8().constData(), 0);
        av_opt_set(ofmt_ctx->priv_data, "reset_timestamps", "1", 0);

        if ( ! (ofmt_ctx->oformat->flags & AVFMT_NOFILE) )
        {
            if (avio_open(&ofmt_ctx->pb, outPattern.toUtf8().constData(), AVIO_FLAG_WRITE) < 0)
                throw std::runtime_error("Could not open output file");
        }

        if (avformat_write_header(ofmt_ctx, nullptr) < 0) throw std::runtime_error("Failed to write header");

        if (a_enc_ctx->frame_size <= 0) a_enc_ctx->frame_size = 1024;

        audio_frame->nb_samples = a_enc_ctx->frame_size;
        audio_frame->format = a_enc_ctx->sample_fmt;
        audio_frame->sample_rate = a_enc_ctx->sample_rate;
#if FFMPEG_NEW_CHANNEL_LAYOUT
        av_channel_layout_default(&audio_frame->ch_layout, 2);
#else
        audio_frame->channel_layout = AV_CH_LAYOUT_STEREO;
        audio_frame->channels = 2;
#endif
        if (av_frame_get_buffer(audio_frame, 0) < 0) throw std::runtime_error("Alloc audio buffer failed");

        av_samples_set_silence(audio_frame->data, 0, audio_frame->nb_samples, 2, static_cast<AVSampleFormat>(audio_frame->format));

        emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::Processing));

        int64_t current_audio_pts = 0;
        int64_t current_video_frames = 0;

        while (av_read_frame(ifmt_ctx, pkt) >= 0)
        {
            {
                QMutexLocker l(&m_mutexAbort);
                if (m_abort) { av_packet_unref(pkt); throw std::runtime_error("Abort requested"); }
            }

            if (pkt->stream_index == video_stream_idx)
            {
                if (avcodec_send_packet(dec_ctx, pkt) >= 0)
                {
                    while (avcodec_receive_frame(dec_ctx, frame) >= 0)
                    {
                        if ( ! filter_graph)
                        {
                            emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::Filtering));
                            // [FIX] Pass YUV420P target
                            this->initFilterGraph(&filter_graph,
                                                  &buffersrc_ctx,
                                                  &buffersink_ctx,
                                                  frame,
                                                  in_stream->time_base,
                                                  pipelineFps,
                                                  AV_PIX_FMT_YUV420P);

                            if (frame->color_range != AVCOL_RANGE_UNSPECIFIED) v_enc_ctx->color_range = frame->color_range;
                            if (frame->colorspace != AVCOL_SPC_UNSPECIFIED) v_enc_ctx->colorspace = frame->colorspace;
                        }

                        if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0)
                        {
                            while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0)
                            {
                                filt_frame->pts = current_video_frames;
                                this->writeEncodedPacket(filt_frame, v_enc_ctx, ofmt_ctx, v_out);
                                current_video_frames++;
                                av_frame_unref(filt_frame);

                                double video_time = (double)current_video_frames / pipelineFps;

                                while ((double)current_audio_pts / a_enc_ctx->sample_rate < video_time)
                                {
                                    ret = av_frame_make_writable(audio_frame);
                                    av_samples_set_silence(audio_frame->data,
                                                           0,
                                                           audio_frame->nb_samples,
                                                           2,
                                                           static_cast<AVSampleFormat>(audio_frame->format));

                                    audio_frame->pts = current_audio_pts;
                                    this->writeEncodedPacket(audio_frame, a_enc_ctx, ofmt_ctx, a_out);
                                    current_audio_pts += audio_frame->nb_samples;
                                }

                                processedFrames++;
                                if (targetTotal > 0)
                                {
                                    int p = static_cast<int>((static_cast<double>(processedFrames) / targetTotal) * 100);
                                    if (p > lastPercent) { lastPercent = p; emit progressUpdated(p); }
                                }
                                this->handleProgressUpdate(processedFrames);
                            }
                        }

                        av_frame_unref(frame);
                    }
                }
            }

            av_packet_unref(pkt);
        }

        // DRAIN THE DECODER: We must pull out all lingering raw frames (which represent
        // the remaining half of your movie) BEFORE we flush the filter graph.
        if (dec_ctx)
        {
            avcodec_send_packet(dec_ctx, nullptr); // Signal EOF to decoder

            while (avcodec_receive_frame(dec_ctx, frame) >= 0)
            {
                if (filter_graph && av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) >= 0)
                {
                    while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0)
                    {
                        filt_frame->pts = current_video_frames;
                        this->writeEncodedPacket(filt_frame, v_enc_ctx, ofmt_ctx, v_out);
                        current_video_frames++;
                        av_frame_unref(filt_frame);

                        double video_time = (double)current_video_frames / pipelineFps;

                        while ((double)current_audio_pts / a_enc_ctx->sample_rate < video_time)
                        {
                            ret = av_frame_make_writable(audio_frame);

                            if (ret < 0) throw std::runtime_error("Failed to make audio frame writable: " +
                                                         LogUtils::ffmpegErrorToString(ret).toStdString());

                            av_samples_set_silence(audio_frame->data, 0, audio_frame->nb_samples, 2, static_cast<AVSampleFormat>(audio_frame->format));

                            audio_frame->pts = current_audio_pts;

                            this->writeEncodedPacket(audio_frame, a_enc_ctx, ofmt_ctx, a_out);

                            current_audio_pts += audio_frame->nb_samples;
                        }
                    }
                }

                av_frame_unref(frame);
            }
        }

        emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::Flushing));

        if (filter_graph)
        {
            int flushRet = av_buffersrc_add_frame_flags(buffersrc_ctx, nullptr, 0);

            if (flushRet < 0 && flushRet != AVERROR_EOF)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog("Warning flushing filter graph: " + LogUtils::ffmpegErrorToString(flushRet),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
            }

            while (av_buffersink_get_frame(buffersink_ctx, filt_frame) >= 0)
            {
                 filt_frame->pts = current_video_frames++;
                 this->writeEncodedPacket(filt_frame, v_enc_ctx, ofmt_ctx, v_out);
                 av_frame_unref(filt_frame);

                 double video_time = (double)current_video_frames / pipelineFps;

                 while ((double)current_audio_pts / a_enc_ctx->sample_rate < video_time)
                 {
                    ret = av_frame_make_writable(audio_frame);

                     if (ret < 0) throw std::runtime_error("Failed to make audio frame writable during flush: " +
                                                  LogUtils::ffmpegErrorToString(ret).toStdString());

                    av_samples_set_silence(audio_frame->data, 0, audio_frame->nb_samples, 2, static_cast<AVSampleFormat>(audio_frame->format));
                    audio_frame->pts = current_audio_pts;
                    this->writeEncodedPacket(audio_frame, a_enc_ctx, ofmt_ctx, a_out);
                    current_audio_pts += audio_frame->nb_samples;
                 }
            }
        }

        this->writeEncodedPacket(nullptr, v_enc_ctx, ofmt_ctx, v_out);
        this->writeEncodedPacket(nullptr, a_enc_ctx, ofmt_ctx, a_out);

        emit statusUpdated(ExtractorUtils::ffmpegStateToString(FFmpegState_t::Finalizing));
        av_write_trailer(ofmt_ctx);

        ret = 0;

    } catch (const std::exception &e) {
        if (exitCode == 0) internalStatus = -1;

#ifdef QT_DEBUG
        LogUtils::safeLog("FFmpeg Error: " + QString::fromStdString(e.what()),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(e.what());
    }

#ifdef QT_DEBUG
    qint64 totalElapsedMs = timer.elapsed();
#endif
    av_packet_free(&pkt);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);
    av_frame_free(&audio_frame);

    if (dec_ctx) avcodec_free_context(&dec_ctx);
    if (v_enc_ctx) avcodec_free_context(&v_enc_ctx);
    if (a_enc_ctx) avcodec_free_context(&a_enc_ctx);
    if (ifmt_ctx) avformat_close_input(&ifmt_ctx);

    if (ofmt_ctx)
    {
        if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
    }

    if (filter_graph) avfilter_graph_free(&filter_graph);

#ifdef QT_DEBUG
    if (ret == 0 && internalStatus == 0)
    {
        LogUtils::safeLog(QString("Pipeline completed successfully in %1ms").arg(totalElapsedMs),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
    }
    else
    {
        LogUtils::safeLog(QString("Pipeline failed or aborted after %1ms").arg(totalElapsedMs),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
    }
#endif

    this->onFinished(ret, internalStatus);
}

void VideoSegmentationWorker::writeEncodedPacket(AVFrame *frame, AVCodecContext *enc, AVFormatContext *ofmt, AVStream *out)
{
    AVPacket *pkt = av_packet_alloc();
    if (!pkt) return;

    int ret = avcodec_send_frame(enc, frame);

//     while (ret == AVERROR(EAGAIN))
//     {
//         int receiveRet = avcodec_receive_packet(enc, pkt);
//         if (receiveRet == AVERROR(EAGAIN) || receiveRet == AVERROR_EOF) break;
//         if (receiveRet < 0)
//         {
// #ifdef QT_DEBUG
//             LogUtils::safeLog("Error receiving packet during EAGAIN drain: " + LogUtils::ffmpegErrorToString(receiveRet),
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
// #endif
//             break;
//         }

//         av_packet_rescale_ts(pkt, enc->time_base, out->time_base);
//         pkt->stream_index = out->index;

//         if (av_interleaved_write_frame(ofmt, pkt) < 0)
//         {
// #ifdef QT_DEBUG
//             LogUtils::safeLog("Error writing packet during EAGAIN drain",
//                               __FILE__,
//                               __LINE__,
//                               Q_FUNC_INFO);
// #endif
//         }

//         av_packet_unref(pkt);

//         ret = avcodec_send_frame(enc, frame);
//     }

    if (ret < 0 && ret != AVERROR_EOF && ret != AVERROR(EAGAIN))
    {
#ifdef QT_DEBUG
        LogUtils::safeLog("Error sending frame to encoder: " + LogUtils::ffmpegErrorToString(ret),
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        av_packet_free(&pkt);
        return;
    }

    while (true)
    {
        ret = avcodec_receive_packet(enc, pkt);

        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;

        if (ret < 0)
        {
#ifdef QT_DEBUG
            LogUtils::safeLog("Error receiving packet: " + LogUtils::ffmpegErrorToString(ret),
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
            break;
        }

        av_packet_rescale_ts(pkt, enc->time_base, out->time_base);

        pkt->stream_index = out->index;

        if (av_interleaved_write_frame(ofmt, pkt) < 0)
        {
#ifdef QT_DEBUG
            LogUtils::safeLog("Error writing packet",
                              __FILE__,
                              __LINE__,
                              Q_FUNC_INFO);
#endif
        }

        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
}

// --- Support Helpers ---

void VideoSegmentationWorker::handleFFmpegError(qint32 errCode, const QString &context)
{
    Q_UNUSED(context);
    QString msg = LogUtils::ffmpegErrorToString(errCode);

#ifdef QT_DEBUG
    LogUtils::safeLog("FFmpeg Error: " + msg,
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    emit error(msg);
}

void VideoSegmentationWorker::handleProgressUpdate(qint32 currentFrame)
{
    qint32 total = totalFramesForUI();

    if (total <= 0) return;

    qint64 currentTime = this->m_elapsedTimer.elapsed();
   this->m_history.enqueue({currentTime, currentFrame});

    if (m_history.size() > 20) this->m_history.dequeue();

    if (m_history.size() > 1)
    {
        auto first = this->m_history.head();
        auto last = this->m_history.last();
        double msPerFrame = (double)(last.timestamp - first.timestamp) / (last.frame - first.frame);
        emit etaUpdated(ExtractorUtils::formatTime((total - currentFrame) * msPerFrame));
    }
}

void VideoSegmentationWorker::onFinished(qint32 exitCode, qint32 internalStatus)
{
    bool wasAborted = false;
    {
        QMutexLocker locker(&this->m_mutexAbort);
        wasAborted = this->m_abort;
    }

    if (exitCode != 0 || internalStatus != 0)
    {
        QString err = (exitCode != 0) ? LogUtils::ffmpegErrorToString(exitCode) : "Internal Error";

#ifdef QT_DEBUG
        LogUtils::safeLog("Finishing with error: " + err,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif

        emit error(err);

    } else
    if (wasAborted)
    {
        ExtractorUtils::cleanupVideoPartialFiles(this->dir(), "segment%1_*.mp4", this->modelCode());

        emit error(tr("Operation cancelled by user."));
    }

    this->m_retryCount = 0;

    this->setProcessLastStatus("Finished");

    emit finished(exitCode, internalStatus);
}

void VideoSegmentationWorker::performAbort()
{
    QMutexLocker locker(&this->m_mutexAbort);
    this->m_abort = true;
}

void VideoSegmentationWorker::restartPipelineWithDelay()
{
    int backoffDelay = 2000 * std::pow(2, this->m_retryCount - 1);

    QTimer::singleShot(backoffDelay, this, [this]() {
        this->runFFmpegPipeline(); }
                       );
}
