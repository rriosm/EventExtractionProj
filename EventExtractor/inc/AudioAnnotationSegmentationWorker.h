#ifndef AUDIOANNOTATIONSEGMENTATIONWORKER_H
#define AUDIOANNOTATIONSEGMENTATIONWORKER_H

#include <QObject>
#include <QMutex>

#include "Url.h"
using namespace utils;

class AudioAnnotationSegmentationWorker : public QObject
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
         * @brief AudioAnnotationSegmentationWorker
         * @param parent
         */
        explicit AudioAnnotationSegmentationWorker(QObject *parent = nullptr);

        /**
         * @brief ~AudioAnnotationSegmentationWorker
         */
        virtual ~AudioAnnotationSegmentationWorker();

        Q_DISABLE_COPY(AudioAnnotationSegmentationWorker);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const AudioAnnotationSegmentationWorker &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const AudioAnnotationSegmentationWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioAnnotationSegmentationWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioAnnotationSegmentationWorker *ptr) const;

        /**
         * @brief clipSec
         * @return
         */
        qreal clipSec() const;

        /**
         * @brief setClipSec
         * @param newClipSec
         */
        void setClipSec(qreal newClipSec);

        /**
         * @brief annotationManifest
         * @return
         */
        Url annotationManifest() const;

        /**
         * @brief setAnnotationManifest
         * @param newAnnotationManifest
         */
        void setAnnotationManifest(const Url &newAnnotationManifest);

        /**
         * @brief annotationClipsOutputDir
         * @return
         */
        QString annotationClipsOutputDir() const;

        /**
         * @brief setAnnotationClipsOutputDir
         * @param newAnnotationClipsOutputDir
         */
        void setAnnotationClipsOutputDir(const QString &newAnnotationClipsOutputDir);

    signals:

        /**
         * @brief errorOccurred
         * @param message
         */
        void errorOccurred(const QString &message);

        /**
         * @brief progressUpdated
         * @param percentage
         */
        void progressUpdated(double percentage);

        /**
         * @brief finished
         */
        void finished(qint32 exitCode);

    public slots:

        /**
         * @brief segmentJsonContent
         * @param annotationJsonPath
         * @param outputDir
         * @param clipSec
         */
        void segmentJsonContent(const QString &annotationJsonPath);

    private:

        /**
         * @brief generateJsonManifest
         */
        void generateJsonManifest();

    private:

        qreal m_clipSec;

        Url m_annotationManifest;

        QString m_annotationClipsOutputDir;

        mutable QMutex m_mutexClipSec;
        mutable QMutex m_mutexAnnotationManifest;
        mutable QMutex m_mutexAnnotationClipsOutputDir;
};

Q_DECLARE_METATYPE(AudioAnnotationSegmentationWorker *)
Q_DECLARE_OPAQUE_POINTER(AudioAnnotationSegmentationWorker *)

#endif // AUDIOANNOTATIONSEGMENTATIONWORKER_H
