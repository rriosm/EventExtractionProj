#ifndef AUDIOSEGMENTATIONWORKER_H
#define AUDIOSEGMENTATIONWORKER_H

#include <QString>
#include <QMutex>
#include <QElapsedTimer>
#include <QQueue>

#include "inc/Defs.h"
#include "inc/IAudioSegmentationWorker.h"

class AudioSegmentationWorker : public IAudioSegmentationWorker
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
         * @brief AudioSegmentationWorker
         * @param parent
         */
        explicit AudioSegmentationWorker(QObject *parent = nullptr);

        /**
         * @brief AudioSegmentationWorker
         * @param input
         * @param dir
         * @param clipSec
         * @param parent
         */
        AudioSegmentationWorker(const QString &input,
                                const QString &dir,
                                qreal clipSec = *Constants::Q_QGS_DEFAULT_CLIP_SEC, // Default to 5 minutes
                                QObject *parent = nullptr);

        /**
         * @brief ~AudioSegmentationWorker
         */
        virtual ~AudioSegmentationWorker();

        Q_DISABLE_COPY(AudioSegmentationWorker)

        /**
         * @brief inputAudio
         * @return
         */
        QString inputAudio() const Q_DECL_OVERRIDE;

        /**
         * @brief setInputAudio
         * @param newVal
         * @return
         */
        bool setInputAudio(const QString &newVal) Q_DECL_OVERRIDE;

        /**
         * @brief outputDir
         * @return
         */
        QString outputDir() const Q_DECL_OVERRIDE;

        /**
         * @brief setOutputDir
         * @param newVal
         * @return
         */
        bool setOutputDir(const QString &newVal) Q_DECL_OVERRIDE;

        /**
         * @brief clipSec
         * @return
         */
        qreal clipSec() const Q_DECL_OVERRIDE;

        /**
         * @brief setClipSec
         * @param newVal
         * @return
         */
        bool setClipSec(qreal newVal) Q_DECL_OVERRIDE;

        /**
         * @brief totalDurationForUI
         * @return
         */
        double totalDurationForUI() const Q_DECL_OVERRIDE;

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
         * @brief performAbort
         */
        void performAbort();

        /**
         * @brief runFFmpegPipeline
         */
        void runFFmpegPipeline();

        /**
         * @brief setTotalDurationForUI
         * @param filePath
         */
        void setTotalDurationForUI(const QString &filePath) Q_DECL_OVERRIDE;

        /**
         * @brief handleProgressUpdate
         * @param currentSeconds
         */
        void handleProgressUpdate(double currentSeconds);

        /**
         * @brief onFinished
         * @param exitCode
         * @param internalStatus
         */
        void onFinished(qint32 exitCode, qint32 internalStatus);

    private:

        QString m_inputAudio;
        QString m_dir;
        qreal m_clipSec;

        double m_totalDurationSec;
        bool m_abort;
        qint32 m_retryCount;
        QString m_lastStatus;

        mutable QMutex m_mutexInputAudio;
        mutable QMutex m_mutexDir;
        mutable QMutex m_mutexClipSec;
        mutable QMutex m_mutexTotalDuration;
        mutable QMutex m_mutexAbort;
        mutable QMutex m_mutexStatus;

        QElapsedTimer m_elapsedTimer;
        QQueue<FFmpegProgress> m_history;
};

Q_DECLARE_METATYPE(AudioSegmentationWorker *)
Q_DECLARE_OPAQUE_POINTER(AudioSegmentationWorker *)

#endif // AUDIOSEGMENTATIONWORKER_H
