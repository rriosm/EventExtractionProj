#ifndef AUDIOANNOTATIONSEGMENTATIONCONTROLLER_H
#define AUDIOANNOTATIONSEGMENTATIONCONTROLLER_H

#include <QObject>
#include <QThread>

#include "inc/PipelineWorker.h"
#include "inc/AudioAnnotationSegmentationWorker.h"

class AudioAnnotationSegmentationController : public PipelineWorker
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
         * @brief AudioAnnotationSegmentationController
         * @param parent
         */
        explicit AudioAnnotationSegmentationController(QObject *parent = nullptr);

        /**
         * @brief ~AudioAnnotationSegmentationController
         */
        virtual ~AudioAnnotationSegmentationController();

        Q_DISABLE_COPY(AudioAnnotationSegmentationController);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const AudioAnnotationSegmentationController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const AudioAnnotationSegmentationController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioAnnotationSegmentationController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioAnnotationSegmentationController *ptr) const;

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
         * @brief progressChanged
         * @param percentage
         */
        void progressChanged(int percentage);

        /**
         * @brief statusMessage
         * @param message
         */
        void statusChanged(QString message);

        /**
         * @brief transcriptionError
         * @param error
         */
        void error(QString msg);

    public slots:

        /**
         * @brief cancelTranscription
         */
        void cancelSegmentation();

        /**
         * @brief handleFinished
         */
        void handleFinished(qint32 exitCode);

        /**
         * @brief handleInput
         * @param info
         */
        void handleInput(FileInfo info) Q_DECL_OVERRIDE;

    private slots:

        /**
         * @brief handleWorkerError
         * @param error
         */
        void handleWorkerError(QString msg);

    private:

        QThread * m_workerThread;

        AudioAnnotationSegmentationWorker * m_worker;

        Url m_annotationManifest;

        QString m_annotationClipsOutputDir;

        qreal m_clipSec;

};

Q_DECLARE_METATYPE(AudioAnnotationSegmentationController *)
Q_DECLARE_OPAQUE_POINTER(AudioAnnotationSegmentationController *)

#endif // AUDIOANNOTATIONSEGMENTATIONCONTROLLER_H
