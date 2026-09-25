#ifndef IVIDEOSEGMENTATIONWORKER_H
#define IVIDEOSEGMENTATIONWORKER_H

#include <QObject>

#include "inc/Defs.h"
#include "inc/VideoSegmentInfo.h"

class IVideoSegmentationWorker : public QObject
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
         * @brief IVideoSegmentationWorker
         * @param parent
         */
        explicit IVideoSegmentationWorker(QObject *parent = nullptr) : QObject(parent) {}

        /**
         * @brief ~IVideoSegmentationWorker
         */
        virtual ~IVideoSegmentationWorker() {}

        // Common Getters/Setters interface

        // --- The Data Contract (Getters) ---

        /**
         * @brief inputVideo
         * @return
         */
        virtual QString inputVideo() const = 0;

        /**
         * @brief setInputVideo
         * @param input
         * @return
         */
        virtual bool setInputVideo(const QString &input) = 0;

        /**
         * @brief dir
         * @return
         */
        virtual QString dir() const = 0;

        /**
         * @brief setDir
         * @param dir
         * @return
         */
        virtual bool setDir(const QString &dir) = 0;

        /**
         * @brief modelCode
         * @return
         */
        virtual ModelCode_t modelCode() const = 0;

        /**
         * @brief setModelCode
         * @param code
         * @return
         */
        virtual bool setModelCode(ModelCode_t code) = 0;

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
         * @brief slowdownFactor
         * @return
         */
        virtual qreal slowdownFactor() const = 0;

        /**
         * @brief setSlowdownFactor
         * @param newSlowdownFactor
         * @return
         */
        virtual bool setSlowdownFactor(qreal newSlowdownFactor) = 0;

        /**
         * @brief totalFramesForUI
         * @return
         */
        virtual qint32 totalFramesForUI() const = 0;

        /**
         * @brief setTotalFrames: "Pre-flight Check"
         *                         Use efore the pipeline starts (UI initialization)
         *                         Lower Efficiency: Must open/close the file from scratch
         *                         Accuracy: Good, but relies on a quick "sniff" of the file.
         *                         Thread Safety: Uses a Mutex to update a shared member variable.
         * @param filePath
         */
        virtual void setTotalFramesForUI(const QString & filePath) = 0;

        /**
         * @brief getDurationViaFFprobe
         * @param filePath
         * @return
         */
        virtual double getDurationViaFFprobe(const QString &filePath) = 0;

        /**
         * @brief processLastsStatus
         * @return
         */
        virtual QString processLastStatus() const = 0;

        /**
         * @brief setProcessLastsStatus
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
        void resultReady(VideoSegmentInfo::VideoSegmentInfoList_t results);

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

#endif
