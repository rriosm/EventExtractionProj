#ifndef VIDEOSEGMENTATIONWORKERCLI_H
#define VIDEOSEGMENTATIONWORKERCLI_H

#include <QObject>
#include <QMutex>
#include <QElapsedTimer>
#include <QQueue>
#include <QProcess>

#include "inc/IVideoSegmentationWorker.h"
#include "inc/VideoModelConfig.h"
#include "inc/VideoSegmentInfo.h"

class VideoSegmentationWorkerCLI : public IVideoSegmentationWorker
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
         * @brief VideoSegmentationWorkerCLI
         * @param parent
         */
        explicit VideoSegmentationWorkerCLI(QObject *parent = nullptr);

        /**
         * @brief VideoSegmentationWorkerCLI
         * @param input
         * @param dir
         * @param model
         * @param clipSec
         * @param parent
         */
        VideoSegmentationWorkerCLI(const QString &input,
                           const QString &dir,
                           const ModelCode_t model,
                           qint32 clipSec,
                           qreal slowdownFactor,
                           QObject *parent = nullptr);

        /**
         * @brief ~VideoSegmentationWorkerCLI
         */
        virtual ~VideoSegmentationWorkerCLI();

        Q_DISABLE_COPY(VideoSegmentationWorkerCLI)

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const VideoSegmentationWorkerCLI & right) const;

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const VideoSegmentationWorkerCLI & right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const VideoSegmentationWorkerCLI *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const VideoSegmentationWorkerCLI *ptr) const;

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
        bool setInputVideo(const QString & newInputVideo) Q_DECL_OVERRIDE;

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
         * @brief setTotalFrames
         * @param filePath
         */
        void setTotalFramesForUI(const QString & filePath) Q_DECL_OVERRIDE;

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

        /**
         * @brief process
         */
        void process() Q_DECL_OVERRIDE;

        /**
         * @brief abort
         */
        void abort() Q_DECL_OVERRIDE;

        /**
         * @brief stop
         */
        void stop() Q_DECL_OVERRIDE;

    private:

        /**
         * Uses ffprobe to extract the duration of a video file in seconds.
         * Returns 0.0 if the process fails or the file is invalid.
         *
         *
         * Arguments:
         *           -v error: Suppresses everything except actual errors.
         *           -show_entries format=duration: Tells ffprobe to only look at the duration field in the container header.
         *           -of default=noprint_wrappers=1:nokey=1: Formats the output as a "flat" string without headers or keys.
         * waitForFinished(3000): Since this is likely being called in a loop (for every segment created), we use a short timeout.
         * toDouble(&ok): This Qt method is safe; if the string contains non-numeric characters, it sets ok to false.
         *
         * Performance Tip
         *      Calling QProcess for every single segment can be slightly slow if you have hundreds of segments. However, because ffprobe only reads the file
         *      header (metadata), it is significantly faster than opening the file in a media player.
         */
        double getDurationViaFFprobe(const QString &filePath) Q_DECL_OVERRIDE;

        /**
         * @brief setupSegmentationCmd: Memory Management: Unlike Python's ffmpeg-python objects, QProcess requires you to build a QStringList of command-line
         *                              arguments.
         * @param inputVideo
         * @param modelCode
         * @param segment_dir
         * @param clipSec
         * @return
         */
        QStringList prepareSegmentationCmdArgs();

        /**
         * @brief prepareTotalFrameArgsCmd
         * @param filePath
         * @return
         */
        QStringList prepareTotalFrameArgsCmd(const QString & filePath);

        /**
         * @brief performAbort
         */
        void performAbort();

    private slots:

        /**
         * @brief readProgress
         */
        void readProgress();

        /**
         * @brief onFinished
         * @param exitCode
         * @param status
         */
        void onFinished(qint32 exitCode, QProcess::ExitStatus status);

        /**
         * @brief onProcessError
         * @param pError
         */
        void onFFmpegProcessError([[maybe_unused]] QProcess::ProcessError pError);

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

        QProcess* m_process = nullptr;

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
        const int m_historyLimit = 15; // Number of samples to average (approx 3-5 seconds)

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

Q_DECLARE_METATYPE(VideoSegmentationWorkerCLI *)
Q_DECLARE_OPAQUE_POINTER(VideoSegmentationWorkerCLI *)

#endif // VIDEOSEGMENTATIONWORKERCLI_H
