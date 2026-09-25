
#include "AudioEtiandRe.h"

#include "Logger.h"
using namespace br::pucrio::telemidia::logger;

#include <QSharedData>


// Source: https://etiand.re/posts/2025/01/how-to-decode-audio-streams-in-c-cpp-using-libav/

extern "C" {
#include <stdio.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/error.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
};

class AudioData : public QSharedData
{
    public:

        AudioData() :
            QSharedData(),
            m_fmt_ctx(nullptr),
            m_codec(nullptr),
            m_packet(av_packet_alloc()),
            m_frame(av_frame_alloc()),
            m_stream(nullptr),
            m_codec_ctx(nullptr),
            m_swr_ctx(nullptr),
            m_ret(0),
            m_buf(nullptr),
            m_dst_nb_samples(0),
            m_max_dst_nb_samples(0),
            m_src_rate(0),
            m_dst_rate(0),
            m_src_nb_channels(0),
            m_dst_nb_channels(0),
            m_dst_linesize(0)
        {
            // TODO
        }

        AudioData(const AudioData &audioData) :
            QSharedData(audioData)
        {
            // TODO
        }

        virtual ~AudioData()
        {
            // TODO
        }

        AVFormatContext *m_fmt_ctx;
        AVCodec *m_codec;
        AVPacket *m_packet;
        AVFrame *m_frame;
        AVStream *m_stream;
        AVCodecContext *m_codec_ctx;
        SwrContext *m_swr_ctx;

        AVSampleFormat m_src_sample_fmt, m_dst_sample_fmt;
        AVChannelLayout *m_src_ch_layout, *m_dst_ch_layout;

        int m_ret;
        uint8_t **m_buf;

        int m_dst_nb_samples;
        int m_max_dst_nb_samples;
        int m_src_rate;
        int m_dst_rate;
        int m_src_nb_channels;
        int m_dst_nb_channels;
        int m_dst_linesize;

        // Let's say the  stream we're interested in is the first one.
        const int STREAM_IDX = 0;

        const size_t CHUNK_NSAMPLES = 1024;

        Url m_mediaFilePath;
};


Audio::Audio(Url mediaPath, QObject *parent) :
    QObject(parent),
    sharedDataP(new AudioData)
{

    this->sharedDataP->m_mediaFilePath = mediaPath;

    this->openAndDecodeContainerFormat(this->sharedDataP->m_mediaFilePath);
}

void Audio::my_custom_audio_processing_function(int16_t *buf, int nsamples, int nchannels, int samplerate)
{
    // Some clever DSP stuff...
}

void Audio::my_other_custom_audio_processing_function(int16_t *buf, int nchannels, int rate)
{
    // Some clever DSP stuff, that assumes that buf always contains 1024 samples

    // Allocate fixed-length buffer, of size CHUNK_NSAMPLES
    uint8_t **chunk = nullptr;

    sharedDataP->m_ret = av_samples_alloc_array_and_samples(&chunk,
                                                          &sharedDataP->m_dst_linesize,
                                                          sharedDataP->m_dst_nb_channels,
                                                          sharedDataP->CHUNK_NSAMPLES,
                                                          sharedDataP->m_dst_sample_fmt,
                                                          0);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Failed to allocate output buffer: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();
    }

    // Allocate the fifo with a bit of space. It will be grown automatically if needed, anyway.
    AVAudioFifo *fifo = av_audio_fifo_alloc(sharedDataP->m_dst_sample_fmt,
                                            sharedDataP->m_dst_nb_channels,
                                            2 * sharedDataP->CHUNK_NSAMPLES);

    sharedDataP->m_ret = av_audio_fifo_write(fifo,
                                           reinterpret_cast<void **>(buf),
                                           sharedDataP->m_dst_nb_samples);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Failed to write samples to audio fifo: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();
    }

    while (av_audio_fifo_size(fifo) >= (sharedDataP->CHUNK_NSAMPLES))
    {
        av_audio_fifo_read(fifo, reinterpret_cast<void **>(chunk), sharedDataP->CHUNK_NSAMPLES);

        my_other_custom_audio_processing_function(chunk,
                                                  sharedDataP->m_dst_nb_channels,
                                                  sharedDataP->m_dst_rate);
    }
}

bool Audio::cleanup()
{
    if (sharedDataP->m_buf) {
        av_freep(&sharedDataP->m_buf[0]);
    }

    av_freep(&sharedDataP->m_buf);
    av_frame_free(&sharedDataP->m_frame);
    av_packet_free(&sharedDataP->m_packet);
    avcodec_free_context(&sharedDataP->m_codec_ctx);
    swr_free(&sharedDataP->m_swr_ctx);
    avformat_close_input(&sharedDataP->m_fmt_ctx);
}

bool Audio::openAndDecodeContainerFormat(Url mediaPath)
{
    // Open the file and populate the AVFormatContext struct, which will contain info on the container format.
    sharedDataP->m_ret = avformat_open_input(&sharedDataP->m_fmt_ctx,
                                             mediaPath.getFilePath().toLatin1().constData(),
                                             NULL,
                                             NULL);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Could not open input file: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    // Some container formats, such as MPEG, don't have a header.
    // This function reads a few packets to fully populate fmt_ctx.
    sharedDataP->m_ret = avformat_find_stream_info(sharedDataP->m_fmt_ctx, NULL);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Could not find stream information: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    // Get a pointer to the desired AVStream, and use the info it contains to initialize the codec.
    sharedDataP->m_stream = sharedDataP->m_fmt_ctx->streams[sharedDataP->STREAM_IDX];

    // Find the corresponding codec for the requested stream
    sharedDataP->m_codec = const_cast<AVCodec *>(avcodec_find_decoder(sharedDataP->m_stream->codecpar->codec_id));

    if ( ! sharedDataP->m_codec)
    {
        fprintf(stderr, "No suitable decoder found for %s\n", avcodec_get_name(sharedDataP->m_stream->codecpar->codec_id));

        this->cleanup();

        return (false);
    }

    // Create the codec context
    sharedDataP->m_codec_ctx = avcodec_alloc_context3(sharedDataP->m_codec);

    if (! sharedDataP->m_codec_ctx )
    {
        fprintf(stderr, "Failed to allocate codec context");

        this->cleanup();

        return (false);
    }

    // Pass the stream info to the codec
    sharedDataP->m_ret = avcodec_parameters_to_context(sharedDataP->m_codec_ctx, sharedDataP->m_stream->codecpar);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Failed to copy codec parameters to codec: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    sharedDataP->m_codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;

    // Initialize the codec
    sharedDataP->m_ret = avcodec_open2(sharedDataP->m_codec_ctx, sharedDataP->m_codec, NULL);

    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Failed to open codec: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    // Allocate the sample converter.
    //  If the codec is already outputting AV_SAMPLE_FMT_S16, then the converter will do nothing.
    sharedDataP->m_src_sample_fmt = sharedDataP->m_codec_ctx->sample_fmt;
    sharedDataP->m_dst_sample_fmt = AV_SAMPLE_FMT_S16;

    // libswresample can also perform resampling and up- and down-mixing, which we’re
    // uninterested in: we simply set the rest of the output settings to the same thing as the input settings.
    sharedDataP->m_src_ch_layout = &sharedDataP->m_codec_ctx->ch_layout;
    sharedDataP->m_dst_ch_layout = sharedDataP->m_src_ch_layout;
    sharedDataP->m_src_nb_channels = sharedDataP->m_codec_ctx->ch_layout.nb_channels;
    sharedDataP->m_dst_nb_channels = sharedDataP->m_src_nb_channels;
    sharedDataP->m_src_rate = sharedDataP->m_codec_ctx->sample_rate;
    sharedDataP->m_dst_rate = sharedDataP->m_src_rate;

    // Initialize the library
    sharedDataP->m_ret = swr_alloc_set_opts2(&sharedDataP->m_swr_ctx,
                                           sharedDataP->m_dst_ch_layout,
                                           sharedDataP->m_dst_sample_fmt,
                                           sharedDataP->m_dst_rate,
                                           sharedDataP->m_src_ch_layout,
                                           sharedDataP->m_src_sample_fmt,
                                           sharedDataP->m_src_rate,
                                           0,
                                           NULL);
    if (sharedDataP->m_ret < 0)
    {
        fprintf(stderr, "Failed to set SwrContext options: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    if ((sharedDataP->m_ret = swr_init(sharedDataP->m_swr_ctx)) < 0)
    {
        fprintf(stderr, "Failed to initialize SwrContext: %s\n", av_err2str(sharedDataP->m_ret));

        this->cleanup();

        return (false);
    }

    // The outer loop iterates on packets. For each of them, we must make sure that it actually corresponds
    // to the stream we’re interested in, and then send it to the codec for decoding.
    while ( av_read_frame(sharedDataP->m_fmt_ctx, sharedDataP->m_packet) >= 0 )
    {
        // Discard the packets that correspond to other streams
        if ( sharedDataP->m_packet->stream_index != sharedDataP->STREAM_IDX )
        {
            av_packet_unref(sharedDataP->m_packet);

            continue;
        }

        // Decode the packet
        if ( (sharedDataP->m_ret = avcodec_send_packet(sharedDataP->m_codec_ctx, sharedDataP->m_packet)) < 0 )
        {
            fprintf(stderr, "Error sending packet for decoding: %s\n", av_err2str(sharedDataP->m_ret));

            break;
        }

        // Each packet may contain more than 1 frame of audio data. This inner loop iterates on them and
        // stores the raw audio data in an AVFrame.
        while ( sharedDataP->m_ret >= 0 )
        {
            sharedDataP->m_ret = avcodec_receive_frame(sharedDataP->m_codec_ctx,
                                                       sharedDataP->m_frame);

            if (sharedDataP->m_ret == AVERROR(EAGAIN))
            {
                break; // we're done with this packet, go to the next one
            }

            if ( sharedDataP->m_ret < 0 )
            {
                fprintf(stderr, "Error during decoding: %s\n", av_err2str(sharedDataP->m_ret));

                this->cleanup();

                return (false);
            }

            // get the number of samples that will be produced by libswr
            sharedDataP->m_dst_nb_samples = swr_get_out_samples(sharedDataP->m_swr_ctx,
                                                                sharedDataP->m_frame->nb_samples);

            // do we need to grow the buffer ?
            if ( sharedDataP->m_dst_nb_samples > sharedDataP->m_max_dst_nb_samples )
            {
                // first, free it if it was previously allocated
                if (sharedDataP->m_buf) {
                    av_freep(&sharedDataP->m_buf[0]);
                }

                av_freep(&sharedDataP->m_buf);

                // do the actual memoryallocation
                sharedDataP->m_ret = av_samples_alloc_array_and_samples(&sharedDataP->m_buf,
                                                                        &sharedDataP->m_dst_linesize,
                                                                        sharedDataP->m_dst_nb_channels,
                                                                        sharedDataP->m_dst_nb_samples,
                                                                        sharedDataP->m_dst_sample_fmt,
                                                                        0);
                if ( sharedDataP->m_ret < 0 )
                {
                    fprintf(stderr, "Failed to allocate output buffer: %s\n", av_err2str(sharedDataP->m_ret));

                    this->cleanup();

                    return (false);
                }

                // store the allocated size in max_dst_nb_samples
                sharedDataP->m_max_dst_nb_samples = sharedDataP->m_dst_nb_samples;
            }

            // sample format conversion of the samples in frame->extended_data. If the codec already outputs
            // the desired format, this will just copy the untouched samples to buf.
            sharedDataP->m_ret = swr_convert(sharedDataP->m_swr_ctx,
                                             sharedDataP->m_buf,
                                             sharedDataP->m_dst_nb_samples,
                                             const_cast<const uint8_t **>(sharedDataP->m_frame->extended_data),
                                             sharedDataP->m_frame->nb_samples);

            if ( sharedDataP->m_ret <= 0 )
            {
                fprintf(stderr, "Failed to convert samples: %s\n", av_err2str(sharedDataP->m_ret));

                this->cleanup();

                return (false);
            }

            my_custom_audio_processing_function(sharedDataP->m_buf,
                                                sharedDataP->m_dst_nb_samples,
                                                sharedDataP->m_dst_nb_channels,
                                                sharedDataP->m_dst_rate);
        }

        av_packet_unref(sharedDataP->m_packet);
    }

    this->cleanup();

    return (true);
}

bool findAudioStream();

bool discoverCodecForDecodingStream();

bool getAudioStreamDetails();

bool allocateOutputBuffer();
