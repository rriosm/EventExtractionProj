
#include "inc/TranscodeWorker.h"

#include "inc/LogUtils.h"

#include "inc/ffmpeg_gen.h"

//extern "C" {
//    #include <libavcodec/avcodec.h>
//    #include <libavformat/avformat.h>
//    #include "libavformat/avio.h"
//    #include "libavutil/audio_fifo.h"
//    #include "libavutil/avassert.h"
//    #include "libavutil/avstring.h"
//    #include <libavutil/channel_layout.h>
//    #include "libavutil/frame.h"
//    #include <libavutil/mem.h>
//    #include <libavutil/opt.h>
//    #include "libswresample/swresample.h"
//}

/* The output bit rate in bit/s */
#define a_96Kbps_OUTPUT_BIT_RATE 96000

// MP3 bit rate
#ifndef MP3_OUPUT_BIT_RATES
#define MP3_128Kbps_OUTPUT_BIT_RATE 128000
#define MP3_256Kbps_OUTPUT_BIT_RATE 256000
#define MP3_320Kbps_OUTPUT_BIT_RATE 320000
#endif

/* The number of output channels */
#ifndef STEREO_OUTPUT_CHANNELS
#define STEREO_OUTPUT_CHANNELS 2
#endif

struct TranscodeContext {
        AVFormatContext *input_format_context = nullptr;
        AVFormatContext *output_format_context = nullptr;
        AVCodecContext *input_codec_context = nullptr;
        AVCodecContext *output_codec_context = nullptr;
        SwrContext *resample_context = nullptr;
        AVAudioFifo *fifo = nullptr;
        qint64 pts = 0;

        // Cleanup function
        void cleanup() {
            if (fifo) av_audio_fifo_free(fifo);

            if (resample_context) swr_free(&resample_context);

            if (output_codec_context) avcodec_free_context(&output_codec_context);

            if (output_format_context)
            {
                if (output_format_context->pb) avio_closep(&output_format_context->pb);

                avformat_free_context(output_format_context);
            }

            if (input_codec_context) avcodec_free_context(&input_codec_context);

            if (input_format_context) avformat_close_input(&input_format_context);

            // Reset pointers to nullptr to avoid double free
            fifo = nullptr;
            resample_context = nullptr;
            output_codec_context = nullptr;
            output_format_context = nullptr;
            input_codec_context = nullptr;
            input_format_context = nullptr;
            pts = 0;
        }
};

// --- Encoding Loop Helpers ---

TranscodeWorker::TranscodeWorker(QObject * parent) :
    QObject(parent),
    m_ctx(new TranscodeContext)
{
    // TODO
}

TranscodeWorker::TranscodeWorker(const Url & inMediaUrl, const Url & outMediaUrl, QObject *parent) :
    QObject(parent),
    m_inMediaUrl(inMediaUrl),
    m_outMediaUrl(outMediaUrl),
    m_ctx(new TranscodeContext)
{
#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Transcode::Transcode(const Url &, const Url &, QObject *) - Entering : %1! ---- %2").
                      arg(this->m_inMediaUrl.path(),
                          this->m_outMediaUrl.path()),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif
}

bool TranscodeWorker::operator ==(const TranscodeWorker &right) const
{
    if ( (this->m_inMediaUrl != right.m_inMediaUrl ) ||
         (this->m_outMediaUrl != right.m_outMediaUrl ) ||
         (this->m_ctx != right.m_ctx ) )
    {
        return false;
    }

    return true;
}

bool TranscodeWorker::operator !=(const TranscodeWorker &right) const
{
    return !(*this == right);
}

bool TranscodeWorker::operator==(const TranscodeWorker *ptr) const
{
    return (this == ptr);
}

bool TranscodeWorker::operator!=(const TranscodeWorker *ptr) const
{
    return (this != ptr);
}

TranscodeWorker::~TranscodeWorker()
{
    this->m_ctx->cleanup();
    delete this->m_ctx;
}

qint32 TranscodeWorker::process()
{
    int audio_stream_idx = -1;
    int ret = 0;
    QString errMsg;

    // =========================================================================
    // 1. Validation and Setup
    // =========================================================================

    if (!this->m_inMediaUrl.isValid() || !this->m_outMediaUrl.isValid())
    {
        errMsg = QString("Transcode::processAudioTranscode() - Input or Output URL is invalid.");
#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(errMsg);
        return AVERROR_EXIT;
    }

    if ((ret = open_input_file(this->m_inMediaUrl.path().toUtf8().constData(), this->m_ctx, &audio_stream_idx)) < 0)
    {
        errMsg = QString("Could not open input: " + getAvErrorMsg(ret));
#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(errMsg);
        return ret;
    }

    if ((ret = open_output_file(this->m_outMediaUrl.path().toUtf8().constData(), this->m_ctx)) < 0)
    {
        errMsg = QString("Could not open output: " + getAvErrorMsg(ret));
#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(errMsg);
        return ret;
    }

    if ((ret = init_resampler(this->m_ctx)) < 0 || (ret = init_fifo(this->m_ctx)) < 0)
    {
        errMsg = QString("Init failed");
#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(errMsg);
        return ret;
    }

    if ((ret = avformat_write_header(this->m_ctx->output_format_context, nullptr)) < 0)
    {
        errMsg = QString("Write Header failed");
#ifdef QT_DEBUG
        LogUtils::safeLog(errMsg,
                          __FILE__,
                          __LINE__,
                          Q_FUNC_INFO);
#endif
        emit error(errMsg);
        return ret;
    }

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Transcode::processAudioTranscode() - Validation passed!"),
                      __FILE__,
                      __LINE__,
                      Q_FUNC_INFO);
#endif

    // =========================================================================
    // 2. Loop Allocations
    // =========================================================================

    AVPacket *input_packet = av_packet_alloc();
    AVFrame *input_frame = av_frame_alloc();
    AVFrame *output_frame = av_frame_alloc();

    // Prepare Output Frame
    output_frame->nb_samples = this->m_ctx->output_codec_context->frame_size;
    output_frame->format = this->m_ctx->output_codec_context->sample_fmt;
    output_frame->sample_rate = this->m_ctx->output_codec_context->sample_rate;
    av_channel_layout_copy(&output_frame->ch_layout, &this->m_ctx->output_codec_context->ch_layout);

    // Buffer Management
    uint8_t **converted_input_samples = nullptr;
    int max_dst_nb_samples = 0;
    int is_decoding_finished = 0;
    int is_resampler_flushed = 0;

#ifdef QT_DEBUG
    LogUtils::safeLog(QString("Transcode::processAudioTranscode() - Loop allocations done!"),
                      __FILE__, __LINE__, Q_FUNC_INFO);
#endif

    double total_duration = 0.0;

    if (this->m_ctx->input_format_context->duration != AV_NOPTS_VALUE)
    {
        // AV_TIME_BASE is FFmpeg's internal microsecond time base
        total_duration = (double)this->m_ctx->input_format_context->duration / AV_TIME_BASE;
    }

    AVStream *in_stream = this->m_ctx->input_format_context->streams[audio_stream_idx];
    int last_reported_percent = -1; // Throttle to prevent flooding the event loop

    while (true)
    {
        const int output_frame_size = this->m_ctx->output_codec_context->frame_size;

//#ifdef QT_DEBUG
//        LogUtils::safeLog(QString("Transcode::processAudioTranscode() - Inside the main loop!"),
//                          __FILE__,
//                          __LINE__,
//                          Q_FUNC_INFO);
//#endif

        // ======================================================================
        // A. Reading Loop (Read -> Decode -> Resample -> FIFO)
        // ======================================================================
        while (av_audio_fifo_size(this->m_ctx->fifo) < output_frame_size)
        {
//#ifdef QT_DEBUG
//            LogUtils::safeLog(QString("Transcode::processAudioTranscode() - Begging of the Reading Loop!"),
//                              __FILE__,
//                              __LINE__,
//                              Q_FUNC_INFO);
//#endif
            // If decoding is finished and we have already flushed the resampler, stop trying to fill FIFO
            if (is_decoding_finished && is_resampler_flushed)
            {
                break;
            }

            // 1. Read Frame (Only if not already finished)
            if (!is_decoding_finished)
            {
                ret = av_read_frame(this->m_ctx->input_format_context, input_packet);

                if (ret < 0)
                {
                    if (ret != AVERROR_EOF)
                    {
#ifdef QT_DEBUG
                        LogUtils::safeLog(QString("Transcode::processAudioTranscode() - Read error: %1. Finishing.")
                                          .arg(getAvErrorMsg(ret)), __FILE__, __LINE__, Q_FUNC_INFO);
#endif
                    }

                    is_decoding_finished = 1;
                } else {

                    if (input_packet->stream_index == audio_stream_idx && total_duration > 0.0)
                    {
                        if (input_packet->pts != AV_NOPTS_VALUE)
                        {
                            // Convert PTS to seconds using the stream's time base
                            double current_time = input_packet->pts * av_q2d(in_stream->time_base);
                            double percent = (current_time / total_duration) * 100.0;

                            // Clamp to safe values
                            if (percent < 0.0) percent = 0.0;
                            if (percent > 100.0) percent = 100.0;

                            // THROTTLE: Only emit if the whole number percentae chagned
                            int current_percent_int = static_cast<int>(percent);
                            if (current_percent_int > last_reported_percent)
                            {
                                last_reported_percent = current_percent_int;

#ifdef QT_DEBUG
                                LogUtils::safeLog(QString("Transcode Progress: %1%").arg(percent, 0, 'f', 2),
                                                  __FILE__,
                                                  __LINE__,
                                                  Q_FUNC_INFO);
#endif

                                emit progressUpdated(percent);
                            }
                        }
                    }
                }
            }

            // 2. Send Packet (Normal or Flush)
            if (is_decoding_finished || input_packet->stream_index == audio_stream_idx)
            {
                if (is_decoding_finished)
                {
                    // Flush Decoder
                    avcodec_send_packet(this->m_ctx->input_codec_context, nullptr);
                }
                else
                {
                    // Send Packet
                    ret = avcodec_send_packet(this->m_ctx->input_codec_context, input_packet);
                    if (ret < 0) {
                        // Optional: Log warning here if needed
                    }
                }

                // 3. Receive Frame Loop
                while (true)
                {
                    ret = avcodec_receive_frame(this->m_ctx->input_codec_context, input_frame);

                    bool decoder_is_empty = (ret == AVERROR_EOF);
                    // Special Case: If flushing and we get EAGAIN, assume decoder is drained
                    if (ret == AVERROR(EAGAIN) && is_decoding_finished)
                    {
                        decoder_is_empty = true;
                    }

                    if (decoder_is_empty)
                    {
                        // Decoder is empty. Time to flush the Resampler?
                        if (is_decoding_finished && !is_resampler_flushed)
                        {
                            int delay = swr_get_delay(this->m_ctx->resample_context, this->m_ctx->output_codec_context->sample_rate);
                            if (delay > 0)
                            {
                                int dst_nb_samples = av_rescale_rnd(delay,
                                                                    this->m_ctx->output_codec_context->sample_rate,
                                                                    this->m_ctx->input_codec_context->sample_rate,
                                                                    AV_ROUND_UP);

                                // Alloc Buffer Logic
                                if (!converted_input_samples || dst_nb_samples > max_dst_nb_samples)
                                {
                                    if (converted_input_samples) {
                                        av_freep(&converted_input_samples[0]);
                                        av_freep(&converted_input_samples);
                                    }
                                    if (av_samples_alloc_array_and_samples(&converted_input_samples, nullptr,
                                                                       this->m_ctx->output_codec_context->ch_layout.nb_channels, dst_nb_samples,
                                                                       this->m_ctx->output_codec_context->sample_fmt, 0) >= 0) {
                                        max_dst_nb_samples = dst_nb_samples;
                                    }
                                }

                                if (converted_input_samples)
                                {
                                    // Convert with nullptr to flush
                                    int flushed_count = swr_convert(this->m_ctx->resample_context, converted_input_samples, dst_nb_samples, nullptr, 0);
                                    if (flushed_count > 0) {
                                        add_samples_to_fifo(this->m_ctx->fifo, converted_input_samples, flushed_count);
                                    }
                                }
                            }
                            is_resampler_flushed = 1;
                        }
                        break; // Exit receive loop
                    }
                    else if (ret == AVERROR(EAGAIN))
                    {
                        break; // Need more input
                    }
                    else if (ret < 0)
                    {
#ifdef QT_DEBUG
                        LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_receive_frame < 0"),
                                          __FILE__, __LINE__, Q_FUNC_INFO);
#endif
                        goto loop_cleanup; // Real error
                    }

                    // 4. Normal Processing (Got a Frame)
                    int dst_nb_samples = av_rescale_rnd(swr_get_delay(this->m_ctx->resample_context, this->m_ctx->input_codec_context->sample_rate) + input_frame->nb_samples,
                                                        this->m_ctx->output_codec_context->sample_rate,
                                                        this->m_ctx->input_codec_context->sample_rate,
                                                        AV_ROUND_UP);

                    // Alloc Buffer Logic
                    if (!converted_input_samples || dst_nb_samples > max_dst_nb_samples)
                    {
                        if (converted_input_samples) {
                            av_freep(&converted_input_samples[0]);
                            av_freep(&converted_input_samples);
                        }
                        if (av_samples_alloc_array_and_samples(&converted_input_samples, nullptr,
                                                           this->m_ctx->output_codec_context->ch_layout.nb_channels, dst_nb_samples,
                                                           this->m_ctx->output_codec_context->sample_fmt, 0) < 0) {
                            goto loop_cleanup;
                        }
                        max_dst_nb_samples = dst_nb_samples;
                    }

                    int converted_count = swr_convert(this->m_ctx->resample_context, converted_input_samples, dst_nb_samples,
                                                      (const uint8_t**)input_frame->extended_data, input_frame->nb_samples);
                    if (converted_count > 0)
                    {
                        add_samples_to_fifo(this->m_ctx->fifo, converted_input_samples, converted_count);
                    }
                }
            }
            av_packet_unref(input_packet);
        }

        // ======================================================================
        // B. Encoding Loop (FIFO -> Encoder -> File)
        // ======================================================================
        while (av_audio_fifo_size(this->m_ctx->fifo) >= output_frame_size ||
               (is_decoding_finished && av_audio_fifo_size(this->m_ctx->fifo) > 0))
        {
            const int frame_size = FFMIN(av_audio_fifo_size(this->m_ctx->fifo), output_frame_size);

            av_frame_unref(output_frame);

//#ifdef QT_DEBUG
//            LogUtils::safeLog(QString("Transcode::processAudioTranscode() - av_audio_fifo_size"),
//                              __FILE__,
//                              __LINE__,
//                              Q_FUNC_INFO);
//#endif

            output_frame->nb_samples = frame_size;
            output_frame->format = this->m_ctx->output_codec_context->sample_fmt;
            output_frame->sample_rate = this->m_ctx->output_codec_context->sample_rate;
            av_channel_layout_copy(&output_frame->ch_layout, &this->m_ctx->output_codec_context->ch_layout);

            // Get buffer
            ret = av_frame_get_buffer(output_frame, 0);
            if (ret < 0)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("Transcode::processAudioTranscode() - av_frame_get_buffer failed: %1")
                                  .arg(getAvErrorMsg(ret)),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
                goto loop_cleanup;
            }

            av_audio_fifo_read(this->m_ctx->fifo, (void **)output_frame->data, frame_size);

            output_frame->pts = this->m_ctx->pts;
            this->m_ctx->pts += frame_size;

            ret = avcodec_send_frame(this->m_ctx->output_codec_context, output_frame);
            if (ret < 0)
            {
#ifdef QT_DEBUG
                LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_send_frame failed: %1")
                                  .arg(getAvErrorMsg(ret)),
                                  __FILE__,
                                  __LINE__,
                                  Q_FUNC_INFO);
#endif
                goto loop_cleanup;
            }

            // Write Packets
            while (true)
            {
                AVPacket *out_pkt = av_packet_alloc();
                ret = avcodec_receive_packet(this->m_ctx->output_codec_context, out_pkt);

//#ifdef QT_DEBUG
//                LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_receive_packet"),
//                                  __FILE__,
//                                  __LINE__,
//                                  Q_FUNC_INFO);
//#endif

                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    av_packet_free(&out_pkt);
                    break;
                }
                else if (ret < 0)
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_receive_packet failed: %1")
                                      .arg(getAvErrorMsg(ret)),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                    av_packet_free(&out_pkt);
                    goto loop_cleanup;
                }

                av_write_frame(this->m_ctx->output_format_context, out_pkt);
                av_packet_free(&out_pkt);
            }
        }

        // ======================================================================
        // C. Final Flush & Exit
        // ======================================================================
        if (is_decoding_finished && is_resampler_flushed && av_audio_fifo_size(this->m_ctx->fifo) == 0)
        {
            // Flush Encoder
            avcodec_send_frame(this->m_ctx->output_codec_context, nullptr);

            while (true)
            {
                AVPacket *out_pkt = av_packet_alloc();
                ret = avcodec_receive_packet(this->m_ctx->output_codec_context, out_pkt);

//#ifdef QT_DEBUG
//                LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_send_frame (Flush)"),
//                                  __FILE__,
//                                  __LINE__,
//                                  Q_FUNC_INFO);
//#endif

                if (ret == AVERROR_EOF)
                {
                    av_packet_free(&out_pkt);
                    break; // Success
                }
                else if (ret < 0 && ret != AVERROR(EAGAIN))
                {
#ifdef QT_DEBUG
                    LogUtils::safeLog(QString("Transcode::processAudioTranscode() - avcodec_receive_packet < 0"),
                                      __FILE__,
                                      __LINE__,
                                      Q_FUNC_INFO);
#endif
                    av_packet_free(&out_pkt);
                    break; // Error during flush, but we can exit
                }

                av_write_frame(this->m_ctx->output_format_context, out_pkt);
                av_packet_free(&out_pkt);
            }

            break; // BREAK MAIN LOOP
        }
    }

loop_cleanup:
    av_write_trailer(this->m_ctx->output_format_context);

    av_packet_free(&input_packet);
    av_frame_free(&input_frame);
    av_frame_free(&output_frame);

    if (converted_input_samples)
    {
        av_freep(&converted_input_samples[0]);
        av_freep(&converted_input_samples);
    }

    emit progressUpdated(100.0);

    emit transcodeReady(this->m_outMediaUrl.path());
    emit finished();
    return 0;
}

int TranscodeWorker::add_samples_to_fifo(AVAudioFifo *fifo,
                                         uint8_t **converted_input_samples,
                                         const int frame_size)
{
    int error;

    if ((error = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + frame_size)) < 0)
        return error;

    if (av_audio_fifo_write(fifo, (void **)converted_input_samples, frame_size) < frame_size)
        return AVERROR_EXIT;

    return 0;
}

qint32 TranscodeWorker::configure_audio_codec(AVCodecContext *avctx, AVCodecID codec_id)
{
    if (!avctx) return AVERROR(EINVAL);

    switch (codec_id)
    {
        // ====================================================================
        // GROUP 1: PCM (Uncompressed Integer/Float)
        // Configuration: Pure raw data. No bitrate config required (implicit).
        // ====================================================================
        case AV_CODEC_ID_PCM_S16LE:
        case AV_CODEC_ID_PCM_S16BE:
        case AV_CODEC_ID_PCM_U16LE:
        case AV_CODEC_ID_PCM_U16BE:
        case AV_CODEC_ID_PCM_S8:
        case AV_CODEC_ID_PCM_U8:
        case AV_CODEC_ID_PCM_MULAW:
        case AV_CODEC_ID_PCM_ALAW:
        case AV_CODEC_ID_PCM_S32LE:
        case AV_CODEC_ID_PCM_S32BE:
        case AV_CODEC_ID_PCM_U32LE:
        case AV_CODEC_ID_PCM_U32BE:
        case AV_CODEC_ID_PCM_S24LE:
        case AV_CODEC_ID_PCM_S24BE:
        case AV_CODEC_ID_PCM_U24LE:
        case AV_CODEC_ID_PCM_U24BE:
        case AV_CODEC_ID_PCM_S24DAUD:
        case AV_CODEC_ID_PCM_ZORK:
        case AV_CODEC_ID_PCM_S16LE_PLANAR:
        case AV_CODEC_ID_PCM_DVD:
        case AV_CODEC_ID_PCM_F32BE:
        case AV_CODEC_ID_PCM_F32LE:
        case AV_CODEC_ID_PCM_F64BE:
        case AV_CODEC_ID_PCM_F64LE:
        case AV_CODEC_ID_PCM_BLURAY:
        case AV_CODEC_ID_PCM_LXF:
        case AV_CODEC_ID_S302M:
        case AV_CODEC_ID_PCM_S8_PLANAR:
        case AV_CODEC_ID_PCM_S24LE_PLANAR:
        case AV_CODEC_ID_PCM_S32LE_PLANAR:
        case AV_CODEC_ID_PCM_S16BE_PLANAR:
        case AV_CODEC_ID_PCM_S64LE:
        case AV_CODEC_ID_PCM_S64BE:
        case AV_CODEC_ID_PCM_F16LE:
        case AV_CODEC_ID_PCM_F24LE:
        case AV_CODEC_ID_PCM_VIDC:
        case AV_CODEC_ID_PCM_SGA:
        {
            // No bitrate setting needed for PCM.
            break;
        }

        // ====================================================================
        // GROUP 2: ADPCM (Adaptive Differential PCM)
        // Configuration: Usually fixed ratio compression.
        // ====================================================================
        case AV_CODEC_ID_ADPCM_IMA_QT:
        case AV_CODEC_ID_ADPCM_IMA_WAV:
        case AV_CODEC_ID_ADPCM_IMA_DK3:
        case AV_CODEC_ID_ADPCM_IMA_DK4:
        case AV_CODEC_ID_ADPCM_IMA_WS:
        case AV_CODEC_ID_ADPCM_IMA_SMJPEG:
        case AV_CODEC_ID_ADPCM_MS:
        case AV_CODEC_ID_ADPCM_4XM:
        case AV_CODEC_ID_ADPCM_XA:
        case AV_CODEC_ID_ADPCM_ADX:
        case AV_CODEC_ID_ADPCM_EA:
        case AV_CODEC_ID_ADPCM_G726:
        case AV_CODEC_ID_ADPCM_CT:
        case AV_CODEC_ID_ADPCM_SWF:
        case AV_CODEC_ID_ADPCM_YAMAHA:
        case AV_CODEC_ID_ADPCM_SBPRO_4:
        case AV_CODEC_ID_ADPCM_SBPRO_3:
        case AV_CODEC_ID_ADPCM_SBPRO_2:
        case AV_CODEC_ID_ADPCM_THP:
        case AV_CODEC_ID_ADPCM_IMA_AMV:
        case AV_CODEC_ID_ADPCM_EA_R1:
        case AV_CODEC_ID_ADPCM_EA_R3:
        case AV_CODEC_ID_ADPCM_EA_R2:
        case AV_CODEC_ID_ADPCM_IMA_EA_SEAD:
        case AV_CODEC_ID_ADPCM_IMA_EA_EACS:
        case AV_CODEC_ID_ADPCM_EA_XAS:
        case AV_CODEC_ID_ADPCM_EA_MAXIS_XA:
        case AV_CODEC_ID_ADPCM_IMA_ISS:
        case AV_CODEC_ID_ADPCM_G722:
        case AV_CODEC_ID_ADPCM_IMA_APC:
        case AV_CODEC_ID_ADPCM_VIMA:
        case AV_CODEC_ID_ADPCM_AFC:
        case AV_CODEC_ID_ADPCM_IMA_OKI:
        case AV_CODEC_ID_ADPCM_DTK:
        case AV_CODEC_ID_ADPCM_IMA_RAD:
        case AV_CODEC_ID_ADPCM_G726LE:
        case AV_CODEC_ID_ADPCM_THP_LE:
        case AV_CODEC_ID_ADPCM_PSX:
        case AV_CODEC_ID_ADPCM_AICA:
        case AV_CODEC_ID_ADPCM_IMA_DAT4:
        case AV_CODEC_ID_ADPCM_MTAF:
        case AV_CODEC_ID_ADPCM_AGM:
        case AV_CODEC_ID_ADPCM_ARGO:
        case AV_CODEC_ID_ADPCM_IMA_SSI:
        case AV_CODEC_ID_ADPCM_ZORK:
        case AV_CODEC_ID_ADPCM_IMA_APM:
        case AV_CODEC_ID_ADPCM_IMA_ALP:
        case AV_CODEC_ID_ADPCM_IMA_MTF:
        case AV_CODEC_ID_ADPCM_IMA_CUNNING:
        case AV_CODEC_ID_ADPCM_IMA_MOFLEX:
        case AV_CODEC_ID_ADPCM_IMA_ACORN:
        case AV_CODEC_ID_ADPCM_XMD:
        {
            // ADPCM is typically handled automatically by the encoder
            // based on the sample format.
            break;
        }

        // ====================================================================
        // GROUP 3: Standard Compressed Codecs (MP3, AAC, AC3)
        // Configuration: Require explicit bitrate.
        // ====================================================================

        // MP2 / MP3
        case AV_CODEC_ID_MP2:
        case AV_CODEC_ID_MP3:
        {
            // Standard: 128kbps (Stereo)
            avctx->bit_rate = 128000;
            break;
        }

        // AAC / M4A
        case AV_CODEC_ID_AAC:
        case AV_CODEC_ID_AAC_LATM:
        {
            // Standard: 128kbps (Stereo)
            avctx->bit_rate = 128000;
            break;
        }

        // AC3 / Dolby Digital
        case AV_CODEC_ID_AC3:
        case AV_CODEC_ID_EAC3: // Dolby Digital Plus
        {
            // Standard: 192kbps (Stereo) to 384kbps (5.1)
            avctx->bit_rate = 192000;
            break;
        }

        // DTS
        case AV_CODEC_ID_DTS:
        {
            // Standard: 768kbps or 1536kbps
            avctx->bit_rate = 768000;
            break;
        }

        // Vorbis / Ogg
        case AV_CODEC_ID_VORBIS:
        {
            // Vorbis is often quality-based (qscale), but if setting bitrate:
            avctx->bit_rate = 128000;
            break;
        }

        // Opus (Newer, better than Vorbis)
        case AV_CODEC_ID_OPUS:
        {
            // Optimized for speech/music. 64k-96k is usually transparent.
            avctx->bit_rate = 96000;
            break;
        }

        // WMA (Windows Media Audio)
        case AV_CODEC_ID_WMAV1:
        case AV_CODEC_ID_WMAV2:
        case AV_CODEC_ID_WMAVOICE:
        case AV_CODEC_ID_WMAPRO:
        case AV_CODEC_ID_WMALOSSLESS: // Bitrate ignored for lossless
        {
            avctx->bit_rate = 128000;
            break;
        }

        // ====================================================================
        // GROUP 4: Speech / Low Bitrate Codecs
        // Configuration: Highly constrained bitrates.
        // ====================================================================

        // AMR (Adaptive Multi-Rate)
        case AV_CODEC_ID_AMR_NB: // Narrowband
        {
            // Restricted to specific modes: 4.75k to 12.2k
            avctx->bit_rate = 12200;
            avctx->sample_rate = 8000; // Mandatory
            break;
        }
        case AV_CODEC_ID_AMR_WB: // Wideband
        {
             // Restricted to specific modes: 6.6k to 23.85k
            avctx->bit_rate = 23850;
            avctx->sample_rate = 16000; // Mandatory
            break;
        }

        // GSM
        case AV_CODEC_ID_GSM:
        case AV_CODEC_ID_GSM_MS:
        {
            avctx->bit_rate = 13000;
            avctx->sample_rate = 8000;
            break;
        }

        // G.723 / G.729 (Telephony)
        case AV_CODEC_ID_G723_1:
        {
             avctx->bit_rate = 6300; // Only 5.3k or 6.3k allowed
             break;
        }
        case AV_CODEC_ID_G729:
        {
             avctx->bit_rate = 8000;
             break;
        }

        // Speex (Legacy, Opus is preferred)
        case AV_CODEC_ID_SPEEX:
        {
             // Wideband default
             avctx->bit_rate = 28000;
             break;
        }

        // ====================================================================
        // GROUP 5: DPCM (Differential PCM)
        // Configuration: Fixed.
        // ====================================================================
        case AV_CODEC_ID_ROQ_DPCM:
        case AV_CODEC_ID_INTERPLAY_DPCM:
        case AV_CODEC_ID_XAN_DPCM:
        case AV_CODEC_ID_SOL_DPCM:
        case AV_CODEC_ID_SDX2_DPCM:
        case AV_CODEC_ID_GREMLIN_DPCM:
        case AV_CODEC_ID_DERF_DPCM:
        case AV_CODEC_ID_WADY_DPCM:
        case AV_CODEC_ID_CBD2_DPCM:
        {
            break;
        }

        // ====================================================================
        // GROUP 6: Lossless Compressed Codecs
        // Configuration: Bitrate setting is usually ignored (VBR).
        // ====================================================================
        case AV_CODEC_ID_FLAC:
        case AV_CODEC_ID_ALAC: // Apple Lossless
        case AV_CODEC_ID_APE:  // Monkey's Audio
        case AV_CODEC_ID_TTA:  // True Audio
        case AV_CODEC_ID_WAVPACK:
        case AV_CODEC_ID_MLP:  // Meridian Lossless (DVD-Audio)
        case AV_CODEC_ID_TRUEHD: // Dolby TrueHD
        case AV_CODEC_ID_SHORTEN:
        {
            // Lossless encoders generally ignore 'bit_rate' or use 'compression_level'.
            // Setting a dummy bitrate doesn't hurt, but isn't used.
            break;
        }

        // ====================================================================
        // GROUP 7: Obscure / Game / Legacy Formats
        // Configuration: Default handling.
        // ====================================================================
        case AV_CODEC_ID_DVAUDIO:
        case AV_CODEC_ID_MACE3:
        case AV_CODEC_ID_MACE6:
        case AV_CODEC_ID_VMDAUDIO:
        case AV_CODEC_ID_MP3ADU:
        case AV_CODEC_ID_MP3ON4:
        case AV_CODEC_ID_WESTWOOD_SND1:
        case AV_CODEC_ID_QDM2:
        case AV_CODEC_ID_COOK:
        case AV_CODEC_ID_TRUESPEECH:
        case AV_CODEC_ID_SMACKAUDIO:
        case AV_CODEC_ID_QCELP:
        case AV_CODEC_ID_DSICINAUDIO:
        case AV_CODEC_ID_IMC:
        case AV_CODEC_ID_MUSEPACK7:
        case AV_CODEC_ID_MUSEPACK8:
        case AV_CODEC_ID_ATRAC3:
        case AV_CODEC_ID_ATRAC3P:
        case AV_CODEC_ID_ATRAC1:
        case AV_CODEC_ID_ATRAC9:
        case AV_CODEC_ID_NELLYMOSER:
        case AV_CODEC_ID_SIPR:
        case AV_CODEC_ID_MP1:
        case AV_CODEC_ID_TWINVQ:
        case AV_CODEC_ID_MP4ALS:
        case AV_CODEC_ID_BINKAUDIO_RDFT:
        case AV_CODEC_ID_BINKAUDIO_DCT:
        case AV_CODEC_ID_QDMC:
        case AV_CODEC_ID_CELT:
        case AV_CODEC_ID_8SVX_EXP:
        case AV_CODEC_ID_8SVX_FIB:
        case AV_CODEC_ID_BMV_AUDIO:
        case AV_CODEC_ID_RALF:
        case AV_CODEC_ID_IAC:
        case AV_CODEC_ID_ILBC:
        case AV_CODEC_ID_COMFORT_NOISE:
        case AV_CODEC_ID_TAK:
        case AV_CODEC_ID_METASOUND:
        case AV_CODEC_ID_PAF_AUDIO:
        case AV_CODEC_ID_ON2AVC:
        case AV_CODEC_ID_DSS_SP:
        case AV_CODEC_ID_CODEC2:
        case AV_CODEC_ID_FFWAVESYNTH:
        case AV_CODEC_ID_SONIC:
        case AV_CODEC_ID_SONIC_LS:
        case AV_CODEC_ID_EVRC:
        case AV_CODEC_ID_SMV:
        case AV_CODEC_ID_DSD_LSBF:
        case AV_CODEC_ID_DSD_MSBF:
        case AV_CODEC_ID_DSD_LSBF_PLANAR:
        case AV_CODEC_ID_DSD_MSBF_PLANAR:
        case AV_CODEC_ID_4GV:
        case AV_CODEC_ID_INTERPLAY_ACM:
        case AV_CODEC_ID_XMA1:
        case AV_CODEC_ID_XMA2:
        case AV_CODEC_ID_DST:
        case AV_CODEC_ID_ATRAC3AL:
        case AV_CODEC_ID_ATRAC3PAL:
        case AV_CODEC_ID_DOLBY_E:
        case AV_CODEC_ID_APTX:
        case AV_CODEC_ID_APTX_HD:
        case AV_CODEC_ID_SBC:
        case AV_CODEC_ID_HCOM:
        case AV_CODEC_ID_ACELP_KELVIN:
        case AV_CODEC_ID_MPEGH_3D_AUDIO:
        case AV_CODEC_ID_SIREN:
        case AV_CODEC_ID_HCA:
        case AV_CODEC_ID_FASTAUDIO:
        case AV_CODEC_ID_MSNSIREN:
        case AV_CODEC_ID_DFPWM:
        case AV_CODEC_ID_BONK:
        case AV_CODEC_ID_MISC4:
        case AV_CODEC_ID_APAC:
        case AV_CODEC_ID_FTR:
        case AV_CODEC_ID_WAVARC:
        case AV_CODEC_ID_RKA:
        case AV_CODEC_ID_AC4:
        case AV_CODEC_ID_OSQ:
        case AV_CODEC_ID_QOA:
        case AV_CODEC_ID_LC3:
        default:
        {
            // Default Fallback
            break;
        }
    }

    return 0;
}

QString TranscodeWorker::getAvErrorMsg(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE];

    av_strerror(errnum, errbuf, sizeof(errbuf));

    return QString::fromUtf8(errbuf);
}

int TranscodeWorker::init_fifo(TranscodeContext *tCtx)
{
    if (!(tCtx->fifo = av_audio_fifo_alloc(tCtx->output_codec_context->sample_fmt,
                                        tCtx->output_codec_context->ch_layout.nb_channels, 1)))
    {
        return AVERROR(ENOMEM);
    }

    return 0;
}

int TranscodeWorker::init_resampler(TranscodeContext *tCtx)
{
    int error;
    // swr_alloc_set_opts2 is preferred in newer FFmpeg versions
    error = swr_alloc_set_opts2(&tCtx->resample_context,
                                &tCtx->output_codec_context->ch_layout,
                                tCtx->output_codec_context->sample_fmt,
                                tCtx->output_codec_context->sample_rate,
                                &tCtx->input_codec_context->ch_layout,
                                tCtx->input_codec_context->sample_fmt,
                                tCtx->input_codec_context->sample_rate,
                                0, nullptr);

    if (error < 0) return error;

    return swr_init(tCtx->resample_context);
}

// --- Helper Functions (Previously in TranscodeData) ---

int TranscodeWorker::open_input_file(const char *filename, TranscodeContext *tCtx, int *audio_stream_idx)
{
    const AVCodec *input_codec;
    int error;

    if ((error = avformat_open_input(&tCtx->input_format_context, filename, nullptr, nullptr)) < 0)
    {
        return error;
    }

    if ((error = avformat_find_stream_info(tCtx->input_format_context, nullptr)) < 0)
    {
        return error;
    }

    *audio_stream_idx = av_find_best_stream(tCtx->input_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, &input_codec, 0);

    if (*audio_stream_idx < 0)
    {
        return *audio_stream_idx; // Error code
    }

    tCtx->input_codec_context = avcodec_alloc_context3(input_codec);

    if (!tCtx->input_codec_context) return AVERROR(ENOMEM);

    // Limit input/decoder threads to 4
    tCtx->input_codec_context->thread_count = 4;

    avcodec_parameters_to_context(tCtx->input_codec_context, tCtx->input_format_context->streams[*audio_stream_idx]->codecpar);

    if ((error = avcodec_open2(tCtx->input_codec_context, input_codec, nullptr)) < 0)
    {
        return error;
    }

    return 0;
}

int TranscodeWorker::open_output_file(const char *filename, TranscodeContext *tCtx)
{
    AVIOContext *output_io_context = nullptr;
    AVStream *stream = nullptr;
    const AVCodec *output_codec = nullptr;
    int error;

    if ((error = avio_open(&output_io_context, filename, AVIO_FLAG_WRITE)) < 0)
    {
        return error;
    }

    if (!(tCtx->output_format_context = avformat_alloc_context()))
    {
        return AVERROR(ENOMEM);
    }

    tCtx->output_format_context->pb = output_io_context;

    if (!(tCtx->output_format_context->oformat = av_guess_format(nullptr, filename, nullptr)))
    {
        return AVERROR_MUXER_NOT_FOUND;
    }

    if (!(tCtx->output_format_context->url = av_strdup(filename)))
    {
        return AVERROR(ENOMEM);
    }

    if (!(output_codec = avcodec_find_encoder(tCtx->output_format_context->oformat->audio_codec)))
    {
        return AVERROR_ENCODER_NOT_FOUND;
    }

    if (!(stream = avformat_new_stream(tCtx->output_format_context, nullptr)))
    {
        return AVERROR(ENOMEM);
    }

    if (!(tCtx->output_codec_context = avcodec_alloc_context3(output_codec))) {
        return AVERROR(ENOMEM);
    }

    // Limit output/encoder threads to 4
    tCtx->output_codec_context->thread_count = 4;

    // --- Encoder Configuration ---
    stream->time_base.den = tCtx->input_codec_context->sample_rate;
    stream->time_base.num = 1;

    // --- DOWNMIX FIX ---
    // Capture the output channels to STEREO_OUTPUT_CHANNELS (defined as 2 in the macro)
    // If input is Mono (1), FFMIN returns 1. If input is 5.1 (6), FFMIN returns 2.
    int target_channels = FFMIN(tCtx->input_codec_context->ch_layout.nb_channels, STEREO_OUTPUT_CHANNELS);

    av_channel_layout_default(&tCtx->output_codec_context->ch_layout, target_channels);
    tCtx->output_codec_context->sample_rate = tCtx->input_codec_context->sample_rate;

    // Select Sample Format (Simplified: Pick first supported)
    const enum AVSampleFormat *supported_fmts = nullptr;

    int num_fmts = 0;

    avcodec_get_supported_config(nullptr, output_codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, (const void **)&supported_fmts, &num_fmts);

    if (supported_fmts)
    {
        tCtx->output_codec_context->sample_fmt = supported_fmts[0];
    } else {
        // Fallback or error
        tCtx->output_codec_context->sample_fmt = AV_SAMPLE_FMT_FLTP;
    }

    // Specific Bitrate logic
    if (tCtx->output_format_context->oformat->audio_codec == AV_CODEC_ID_MP3)
    {
        tCtx->output_codec_context->bit_rate = 128000;
    }

    if (tCtx->output_format_context->oformat->flags & AVFMT_GLOBALHEADER)
    {
        tCtx->output_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if ((error = avcodec_open2(tCtx->output_codec_context, output_codec, nullptr)) < 0)
    {
        return error;
    }

    if ((error = avcodec_parameters_from_context(stream->codecpar, tCtx->output_codec_context)) < 0)
    {
        return error;
    }

    return 0;
}

