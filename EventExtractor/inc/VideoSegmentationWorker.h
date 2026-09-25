#ifndef VIDEOSEGMENTATIONWORKER_H
#define VIDEOSEGMENTATIONWORKER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QElapsedTimer>
#include <QQueue>

#include "inc/ffmpeg_gen.h"

#include "inc/IVideoSegmentationWorker.h"

class VideoSegmentationWorker : public IVideoSegmentationWorker
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

    public:

        /**
         * @brief VideoSegmentationWorker
         * @param parent
         */
        explicit VideoSegmentationWorker(QObject *parent = nullptr);

        /**
         * @brief VideoSegmentationWorker
         * @param input
         * @param dir
         * @param model
         * @param clipSec
         * @param parent
         */
        VideoSegmentationWorker(const QString &input,
                                const QString &dir,
                                const ModelCode_t model,
                                qint32 clipSec,
                                qreal slowdownFactor,
                                QObject *parent = nullptr);

        /**
         * @brief ~VideoSegmentationWorker
         */
        virtual ~VideoSegmentationWorker();

        Q_DISABLE_COPY(VideoSegmentationWorker)

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const VideoSegmentationWorker & right) const;

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const VideoSegmentationWorker & right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const VideoSegmentationWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const VideoSegmentationWorker *ptr) const;

        /**
         * @brief inputVideo
         * @return
         */
        QString inputVideo() const Q_DECL_OVERRIDE;

        /**
         * @brief setInputVideo
         * @param newInputVideo
         * @return
         */
        bool setInputVideo(const QString &newInputVideo) Q_DECL_OVERRIDE;

        /**
         * @brief modelCode
         * @return
         */
        ModelCode_t modelCode() const Q_DECL_OVERRIDE;

        /**
         * @brief setModelCode
         * @param newModel
         * @return
         */
        bool setModelCode(ModelCode_t newModel) Q_DECL_OVERRIDE;

        /**
         * @brief dir
         * @return
         */
        QString dir() const Q_DECL_OVERRIDE;

        /**
         * @brief setDir
         * @param newDir
         * @return
         */
        bool setDir(const QString &newDir) Q_DECL_OVERRIDE;

        /**
         * @brief clipSec
         * @return
         */
        qreal clipSec() const Q_DECL_OVERRIDE;

        /**
         * @brief setClipSec
         * @param newClipSec
         * @return
         */
        bool setClipSec(qreal newClipSec) Q_DECL_OVERRIDE;

        /**
         * @brief slowdownFactor
         * @return
         */
        qreal slowdownFactor() const Q_DECL_OVERRIDE;

        /**
         * @brief setSlowdownFactor
         * @param newSlowdownFactor
         * @return
         */
        bool setSlowdownFactor(qreal newSlowdownFactor) Q_DECL_OVERRIDE;

        /**
         * @brief totalFrames
         * @return
         */
        qint32 totalFramesForUI() const Q_DECL_OVERRIDE;

        /**
         * @brief setTotalFrames: "Pre-flight Check"
         *                         Use efore the pipeline starts (UI initialization)
         *                         Lower Efficiency: Must open/close the file from scratch
         *                         Accuracy: Good, but relies on a quick "sniff" of the file.
         *                         Thread Safety: Uses a Mutex to update a shared member variable.
         * @param filePath
         */
        void setTotalFramesForUI(const QString &filePath) Q_DECL_OVERRIDE;

        /**
         * @brief getDurationViaFFprobe
         * @param filePath
         * @return
         */
        double getDurationViaFFprobe(const QString &filePath) Q_DECL_OVERRIDE;

        /**
         * @brief processLastsStatus
         * @return
         */
        QString processLastStatus() const Q_DECL_OVERRIDE;

        /**
         * @brief setProcessLastStatus
         * @param lastStatus
         */
        bool setProcessLastStatus(const QString &lastStatus) Q_DECL_OVERRIDE;

    public slots:

        // Core control

        /**
         * @brief process
         */
        void process() Q_DECL_OVERRIDE;

        /**
         * @brief stop
         */
        void stop() Q_DECL_OVERRIDE;

        /**
         * @brief abort
         */
        void abort() Q_DECL_OVERRIDE;

    private:

        /**
         * @brief calculateTotalFrames: Use during pipeline setup (inside the worker thread).
         *                              Higher Efficiency: Reuses the already-open AVFormatContext.
         *                              Accuracy: Excellent, uses the exact context the pipeline uses.
         *                              Thread Safety: Returns a local value to be used by the caller.
         * @param fmt_ctx
         * @param videoStreamIndex
         * @return
         */
        qint64 calculateTotalFrames(AVFormatContext *fmt_ctx, qint32 videoStreamIndex);

        /**
         * @brief getFrameCount: Contains the math logic of frame count.
         * @param pFormatCtx
         * @param videoStreamIdx
         * @return
         */
        qint64 getFrameCount(AVFormatContext *pFormatCtx, int videoStreamIdx);

        /**
         * @brief initFilterGraph
         * @param graph
         * @param src
         * @param sink
         * @param frame
         * @param inputTimeBase
         * @param fps
         * @param targetPixFmt
         */
        void initFilterGraph(AVFilterGraph **graph,
                             AVFilterContext **src,
                             AVFilterContext **sink,
                             AVFrame *frame,
                             AVRational inputTimeBase,
                             double fps,
                             AVPixelFormat targetPixFmt);

        /**
         * @brief runFFmpegPipeline
         */
        void runFFmpegPipeline();

        /**
         * @brief writeEncodedPacket
         * @param frame
         * @param enc
         * @param ofmt
         * @param out
         */
        void writeEncodedPacket(AVFrame *frame, AVCodecContext *enc, AVFormatContext *ofmt, AVStream *out);

        // --- Support Helpers ---

        /**
         * @brief handleFFmpegError
         * @param errCode
         * @param context
         */
        void handleFFmpegError(qint32 errCode, const QString &context);

        /**
         * @brief handleProgressUpdate
         * @param currentFrame
         */
        void handleProgressUpdate(qint32 currentFrame);

        /**
         * @brief onFinished
         * @param exitCode
         * @param status
         */
        void onFinished(qint32 exitCode, qint32 internalStatus);

        /**
         * @brief performAbort
         */
        void performAbort();

        /**
         * @brief restartPipelineWithDelay: // Helper to manage timing
         */
        void restartPipelineWithDelay();

    private:

        // --- State Variables ---
        QString m_inputVideo;
        QString m_dir;
        ModelCode_t m_modelCode = ModelCode_t::None;
        qreal m_clipSec = 0.00;
        qreal m_slowdownFactor = *Constants::Q_QGS_DEFAULT_SLOWDOWN_FACTOR;
        qint32 m_totalFrames = 0;
        QString m_lastStatus;
        bool m_abort = false;

        qint32 m_retryCount = 0;
        const int m_maxRetries = 3;

        // --- UI/Progress Tracking ---
        /**
         * @brief Structure for progress history tracking
         */
        struct ProgressSnapshot {
            qint64 timestamp;
            qint32 frame;
        };

        QElapsedTimer m_elapsedTimer;
        QQueue<ProgressSnapshot> m_history;
        const qint32 m_historyLimit = 15;

        // --- Thread Safety ---
        mutable QMutex m_mutexInputVideo;
        mutable QMutex m_mutexDir;
        mutable QMutex m_mutexModel;
        mutable QMutex m_mutexClipSec;
        mutable QMutex m_mutexSlowdownFactor;
        mutable QMutex m_mutexAbort;
        mutable QMutex m_mutexStatus;
        mutable QMutex m_mutexTotalFrames;
};

Q_DECLARE_METATYPE(VideoSegmentationWorker *)
Q_DECLARE_OPAQUE_POINTER(VideoSegmentationWorker *)

#endif // VIDEOSEGMENTATIONWORKER_H
