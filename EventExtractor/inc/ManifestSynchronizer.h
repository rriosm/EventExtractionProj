#ifndef MANIFESTSYNCHRONIZER_H
#define MANIFESTSYNCHRONIZER_H

#include <QObject>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QString>
#include <QVector>

#include "inc/Defs.h"
#include "inc/VideoSegmentInfo.h"
#include "inc/PipelineWorker.h"

/**
 * @brief Synchronizes video, audio, annotation, and transcription (VTT) manifests
 *
 * This class merges three segmentation manifest files into the video manifest base:
 * - video_segmentation_manifest.json (base)
 * - audio_segmentation_manifest.json (audio track data)
 * - annotation_manifest.json (annotations)
 * - vtt_segmentation_manifest.json (transcriptions)
 */
class ManifestSynchronizer : public PipelineWorker
{
        Q_OBJECT
        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Controller for managing asynchronous MP3 audio segmentation")

        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief ManifestSynchronizer
         */
        explicit ManifestSynchronizer(PipelineWorker * parent = nullptr);

        virtual ~ManifestSynchronizer();

        /**
         * @brief synchronizeManifests: Synchronizes all manifest files and saves the result
         * @param allData
         * @return
         */
        bool synchronizeManifests(QVector<FileInfo> allData);

        /**
         * @brief parseSynchronizedManifest: Parses a synchronized JSON manifest into a list of VideoSegmentInfo objects
         * @param manifestPath: Path to the synchronized JSON manifest
         * @return A list of VideoSegmentInfo objects
         */
        VideoSegmentInfo::VideoSegmentInfoList_t parseSynchronizedManifest(const QString &manifestPath);

        /**
         * @brief lastError: Gets the error message from the last operation
         * @return
         */
        QString lastError() const;

        /**
         * @brief setLastError
         * @param newLastError
         */
        void setLastError(const QString &newLastError);

        /**
         * @brief getSynchronizedSegmentCount: Gets the number of synchronized segments
         * @return
         */
        int getSynchronizedSegmentCount() const;

        /**
         * @brief outputPath
         * @return
         */
        QString outputPath() const;

        /**
         * @brief setOutputPath
         * @param newOutputPath
         */
        void setOutputPath(const QString &newOutputPath);

    public slots:

        /**
         * @brief handleInput
         * @param info
         */
        void handleInput(FileInfo info) Q_DECL_OVERRIDE;

    signals:

        /**
         * @brief collecitonComplete
         * @param allData
         */
        void collectionComplete(QVector<FileInfo> allData);

        /**
         * @brief error
         * @param errMsg
         */
        void error(QString & errMsg);

    private:

        /**
         * @brief loadJsonFile
         * @param filePath
         * @param jsonDoc
         * @return
         */
        bool loadJsonFile(const QString &filePath, QJsonDocument &jsonDoc);

        /**
         * @brief mergeAudioSegment
         * @param videoSegment
         * @param audioSegment
         */
        void mergeAudioSegment(QJsonObject &videoSegment, const QJsonObject &audioSegment);

        /**
         * @brief mergeAnnotationSegment
         * @param videoSegment
         * @param annotationSegment
         */
        void mergeAnnotationSegment(QJsonObject &videoSegment, const QJsonObject &annotationSegment);

        /**
         * @brief mergeVttSegment
         * @param videoSegment
         * @param vttSegment
         */
        void mergeVttSegment(QJsonObject &videoSegment, const QJsonObject &vttSegment);

        /**
         * @brief checkAndRun
         */
        // void checkAndRun();

    private:

        // QString m_videoPath;
        // QString m_audioPath;
        // QString m_annotationPath;
        // QString m_vttPath;
        QString m_outputPath;

        QString m_lastError;

        qint32 m_synchronizedSegmentCount;

        quint8 m_currentMask = 0; // Initialize to 0 in constructor

        QVector<FileInfo> m_collectedData;

        mutable QMutex m_mutexCollectedData;

        mutable QMutex m_mutexSynchronizedSegmentCount;

        mutable QMutex m_mutexLastError;
};

#endif // MANIFESTSYNCHRONIZER_H
