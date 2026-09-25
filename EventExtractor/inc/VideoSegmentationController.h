#ifndef VIDEOSEGMENTATIONCONTROLLER_H
#define VIDEOSEGMENTATIONCONTROLLER_H

#include <QObject>
#include <QThread>

#include "inc/PipelineWorker.h"
#include "inc/VideoSegmentModel.h"
#include "inc/IVideoSegmentationWorker.h"

class VideoSegmentationController : public PipelineWorker
{
        Q_OBJECT

        // Define properties if you want to use this with QML or Property Animation
        Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        explicit VideoSegmentationController(PipelineWorker *parent = nullptr);

        /**
         * @brief VideoSegmentationController
         * @param input
         * @param model
         * @param dir
         * @param clipSec
         * @param segModeCode
         * @param parent
         */
        VideoSegmentationController(const QString & input,
                                    const ModelCode_t & model,
                                    const QString & dir,
                                    const qreal& clipSec,
                                    const SegmentationMode_t segModeCode,
                                    PipelineWorker *parent = nullptr);

        Q_DISABLE_COPY(VideoSegmentationController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const VideoSegmentationController & right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const VideoSegmentationController & right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const VideoSegmentationController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const VideoSegmentationController *ptr) const;

        /**
         * @brief ~VideoProcessor
         */
        virtual ~VideoSegmentationController();

        /**
         * @brief inputVideo
         * @return
         */
        QString inputVideo() const;

        /**
         * @brief setInputVideo
         * @param inputPath
         * @return
         */
        bool setInputVideo(const QString &newInputVideo);

        /**
         * @brief modelCode
         * @return
         */
        ModelCode_t modelCode() const;

        /**
         * @brief setModelCode
         * @param code
         */
        bool setModelCode(const ModelCode_t code);

        /**
         * @brief outputDir
         * @return
         */
        QString outputDir() const;

        /**
         * @brief setOutputDir
         * @param dir
         */
        bool setOutputDir(const QString &dir);

        /**
         * @brief clipSec
         * @return
         */
        qreal clipSec() const;

        /**
         * @brief setClipSec
         * @param clipSec
         */
        bool setClipSec(const qreal clipSec);

        /**
         * @brief videoSegModel
         * @return
         */
        VideoSegmentModel * videoSegModel() const;

        /**
         * @brief videoSegModel: Access to the Model for the QTableView
         * @return
         */
        QSharedPointer<VideoSegmentModel> videoSegModelClone() const;

        /**
         * @brief segModeCode
         * @return
         */
        SegmentationMode_t segModeCode() const;

        /**
         * @brief setSegModeCode
         * @param segModeCode
         * @return
         */
        bool setSegModeCode(const SegmentationMode_t segModeCode);

        /**
         * @brief status
         * @return
         */
        QString status();

        /**
         * @brief setupVideoProcessor
         * @param modelCode
         * @param segment_dir
         * @param clipSec
         */
        void setupVideoCutter(const ModelCode_t & modelCode,
                              const QString & segment_dir,
                              const qint32 & clipSec,
                              const qreal & slowdonwFactor,
                              const SegmentationMode_t segModeCode);

        /**
         * @brief slowdown_factor
         * @return
         */
        qreal slowdown_factor() const;

        /**
         * @brief setSlowdown_factor
         * @param newSlowdown_factor
         */
        void setSlowdown_factor(qreal newSlowdown_factor);

    signals:

        /**
         * @brief progressUpdated
         * @param percent
         */
        void progressUpdated(qint32 percent);

        /**
         * @brief error
         * @param msg
         */
        void error(QString msg);

        /**
         * @brief requestAbort
         */
        void requestAbort();

        /**
         * @brief statusChanged
         * @param nesStatus
         */
        void statusChanged(const QString &nesStatus);

    public slots:

        /**
         * @brief handleInput
         * @param info
         */
        virtual void handleInput(FileInfo info) Q_DECL_OVERRIDE;

        /**
         * @brief clearResults
         */
        void clearResults();

        /**
         * @brief stop
         */
        void cancelSegmentation();

        /**
         * @brief setStatus
         * @param status
         */
        void setStatus(const QString &status);

    private slots:

        /**
         * @brief onWOrkerFinished
         * @param exitCode
         * @param status
         */
        void handleFinished(qint32 exitCode, qint32 status);

        /**
         * @brief handleError
         * @param error
         */
        void handleError(QString error);

    private:

        /**
         * @brief exportVideoSegmentsToJson: Serializes the segment metadata to a formatted JSON file.
         * @param results: The verified list of video segments.
         * @param outputPath: The absolute path to save the .json file.
         * @param sourceVideoName: Optional identifier for the original match video.
         * @param generatorVersion: The version of the class that calls this function
         * @return true if successful, false otherwise.
         */
        bool exportVideoSegmentsToJson(const VideoSegmentInfo::VideoSegmentInfoList_t &results,
                                       const QString &outputPath,
                                       const QString &sourceVideoName,
                                       quint16 generatorVersion = quint16(0));

        /**
         * @brief segmentsMetadata:
         *                  Regex Replacement: Python's string splitting path.split('_')[-1] is
         *                                     replaced by QRegularExpression for safer ID extraction
         *                                     in C++.
         *                  Duration: Python's get_video_duration is likely a wrapper. In C++, you
         *                            would implement getDurationViaFFprobe by running another QProcess
         *                            with ffprobe -v error -show_entries format=duration -of
         *                            default=noprint_wrappers=1:nokey=1.
         * @param modelCode
         * @param segment_dir
         * @return
         */
        VideoSegmentInfo::VideoSegmentInfoList_t getSegmentsMetadata(const ModelCode_t & modelCode,
                                                                     const QString & segment_dir);

        /**
         * @brief updateView
         */
        void updateView();

    private:

        QThread * m_workerThread;

        IVideoSegmentationWorker * m_worker;

        QString m_inputVideo;
        ModelCode_t m_model;
        QString m_dir;
        qreal m_clipSec;
        QString m_status;
        QSharedPointer<VideoSegmentModel> m_videoSegModel;
        qint64 m_lastProcessingDurationMs = 0;
        SegmentationMode_t m_segModeCode;
        QElapsedTimer m_timer;
        qreal m_slowdown_factor;

};

Q_DECLARE_METATYPE(VideoSegmentationController *)
Q_DECLARE_OPAQUE_POINTER(VideoSegmentationController *)

#endif // VIDEOSEGMENTATIONCONTROLLER_H
