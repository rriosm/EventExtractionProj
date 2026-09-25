#ifndef TRANSCODEWORKER_H
#define TRANSCODEWORKER_H

#include <QObject>
#include <QString>
#include <QMetaType>

#include "Url.h"
using namespace utils;

// Forward declaration of the private implementation struct
struct TranscodeContext;
struct AVCodecContext;

extern "C" {
#include <libavcodec/codec_id.h>
#include "libavutil/audio_fifo.h"
#include <libavutil/avutil.h>
}

#ifndef UINT8_MAX
typedef unsigned char uint8_t;
#endif

class TranscodeWorker : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public: // Function members

        /**
         * @brief Transcode
         * @param parent
         */
        explicit TranscodeWorker(QObject *parent = nullptr);

        /**
         * @brief Transcode
         * @param inMediaUrl
         * @param outMedia
         * @param parent
         */
        explicit TranscodeWorker(const Url & inMediaUrl, const Url & outMedia, QObject *parent = nullptr);

        /**
         * @brief ~Transcode
         */
        virtual ~TranscodeWorker();

        // Disable Copying (CRITICAL for FFmpeg resource safety)
        Q_DISABLE_COPY(TranscodeWorker)

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const TranscodeWorker &right) const;

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const TranscodeWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const TranscodeWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const TranscodeWorker *ptr) const;

    signals:

        /**
         * @brief error
         * @param message
         */
        void error(const QString & message);

        /**
         * @brief finished
         */
        void finished();

        /**
         * @brief progressUpdated
         * @param percent
         */
        void progressUpdated(double percent);

        /**
         * @brief transcodeReady
         * @param filePath
         */
        void transcodeReady(const QString & filePath);

    public slots:

        /**
         * @brief processAudioTranscode
         * @return
         */
        qint32 process();

    private: // Function members

        /**
         * @brief add_samples_to_fifo
         * @param fifo
         * @param converted_input_samples
         * @param frame_size
         * @return
         */
        static int add_samples_to_fifo(AVAudioFifo *fifo,
                                       uint8_t **converted_input_samples,
                                       const int frame_size);

        /**
         * @brief configure_audio_codec
         * @param avctx
         * @param codec_id
         * @return
         */
        qint32 configure_audio_codec(AVCodecContext *avctx, AVCodecID codec_id);

        /**
         * @brief getAvErrorMsg
         * @param errnum
         * @return
         */
        static QString getAvErrorMsg(int errnum);

        /**
         * @brief init_fifo
         * @param tCtx
         * @return
         */
        static int init_fifo(TranscodeContext *tCtx);

        /**
         * @brief init_resampler
         * @param tCtx
         * @return
         */
        static int init_resampler(TranscodeContext *tCtx);

        /**
         * @brief open_input_file
         * @param filename
         * @param tCtx
         * @param audio_stream_idx
         * @return
         */
        static int open_input_file(const char *filename, TranscodeContext *tCtx, int *audio_stream_idx);

        /**
         * @brief open_output_file
         * @param filename
         * @param tCtx
         * @return
         */
        static int open_output_file(const char *filename, TranscodeContext *tCtx);

    private: // Data members

        Url m_inMediaUrl;
        Url m_outMediaUrl;

        // Opaque pointer to hold FFmpeg structs (PIMPL)
        TranscodeContext* m_ctx;
};

//Q_DECLARE_METATYPE(Transcode)
Q_DECLARE_METATYPE(TranscodeWorker *)
Q_DECLARE_OPAQUE_POINTER(TranscodeWorker *)

#endif // TRANSCODEWORKER_H
