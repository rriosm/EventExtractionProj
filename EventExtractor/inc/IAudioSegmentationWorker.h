#ifndef IAUDIOSEGMENTATIONWORKER_H
#define IAUDIOSEGMENTATIONWORKER_H

#include <QObject>

#include "inc/Audio.h"

class IAudioSegmentationWorker : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Interface for background audio segmentation using FFmpeg")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief IAudioSegmentationWorker
         * @param parent
         */
        explicit IAudioSegmentationWorker(QObject *parent = nullptr) : QObject(parent) {}

        /**
         * @brief ~IAudioSegmentationWorker
         */
        virtual ~IAudioSegmentationWorker() {}

        // Common Getters/Setters interface

        // --- The Data Contract (Getters) ---

        /**
         * @brief inputAudio
         * @return
         */
        virtual QString inputAudio() const = 0;

        /**
         * @brief setInputAudio
         * @param input
         * @return
         */
        virtual bool setInputAudio(const QString &input) = 0;

        /**
         * @brief outputDir
         * @return
         */
        virtual QString outputDir() const = 0;

        /**
         * @brief setOutputDir
         * @param dir
         * @return
         */
        virtual bool setOutputDir(const QString &dir) = 0;

        /**
         * @brief clipSec
         * @return
         */
        virtual qreal clipSec() const = 0;

        /**
         * @brief setClipSec
         * @param sec
         * @return
         */
        virtual bool setClipSec(qreal sec) = 0;

        /**
         * @brief totalDurationForUI
         * @return Returns the total duration in seconds. Audio relies on time rather than frame counts.
         */
        virtual double totalDurationForUI() const = 0;

        /**
         * @brief setTotalDurationForUI: "Pre-flight Check"
         *                         Use before the pipeline starts (UI initialization)
         *                         Lower Efficiency: Must open/close the file from scratch
         *                         Accuracy: Good, but relies on a quick "sniff" of the file.
         *                         Thread Safety: Uses a Mutex to update a shared member variable.
         * @param filePath
         */
        virtual void setTotalDurationForUI(const QString & filePath) = 0;

        /**
         * @brief getDurationViaFFprobe
         * @param filePath
         * @return
         */
        virtual double getDurationViaFFprobe(const QString &filePath) = 0;

        /**
         * @brief processLastStatus
         * @return
         */
        virtual QString processLastStatus() const = 0;

        /**
         * @brief setProcessLastStatus
         * @return
         */
        virtual bool setProcessLastStatus(const QString &lastStatus) = 0;

    public slots:

        /**
         * @brief process
         */
        virtual void process() = 0;

        /**
         * @brief stop
         */
        virtual void stop() = 0;

        /**
         * @brief abort
         */
        virtual void abort() = 0;

    signals:

        /**
         * @brief progressUpdated
         * @param percentage
         */
        void progressUpdated(qint32 percentage);

        /**
         * @brief etaUpdated
         * @param eta
         */
        void etaUpdated(const QString &eta);

        /**
         * @brief resultReady
         * @param results
         */
        void resultReady(Audio::AudioList_t results);

        /**
         * @brief error
         * @param message
         */
        void error(const QString &message);

        /**
         * @brief finished
         */
        void finished(qint32 exitCode, qint32 status);

        /**
         * @brief statusUpdated
         * @param message
         */
        void statusUpdated(const QString &message);
};

#endif // IAUDIOSEGMENTATIONWORKER_H
