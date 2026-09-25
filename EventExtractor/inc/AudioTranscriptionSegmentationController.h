#ifndef AUDIOTRANSCRIPTIONSEGMENTATIONCONTROLLER_H
#define AUDIOTRANSCRIPTIONSEGMENTATIONCONTROLLER_H

#include <QThread>

#include "Url.h"
using namespace utils;

#include "inc/PipelineWorker.h"
#include "inc/AudioTranscriptionSegmentationWorker.h"

class AudioTranscriptionSegmentationController : public PipelineWorker
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

        explicit AudioTranscriptionSegmentationController(QObject *parent = nullptr);

        virtual ~AudioTranscriptionSegmentationController();

        Q_DISABLE_COPY(AudioTranscriptionSegmentationController);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const AudioTranscriptionSegmentationController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const AudioTranscriptionSegmentationController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioTranscriptionSegmentationController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioTranscriptionSegmentationController *ptr) const;

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
         * @brief lineLength
         * @return
         */
        qint32 lineLength() const;

        /**
         * @brief setLineLength
         * @param newLineLength
         */
        void setLineLength(qint32 newLineLength);

        /**
         * @brief transcriptionManifest
         * @return
         */
        Url transcriptionManifest() const;

        /**
         * @brief setTranscriptionManifest
         * @param newTranscriptionManifest
         */
        void setTranscriptionManifest(const Url &newTranscriptionManifest);

        /**
         * @brief transcriptionClipsOutputDir
         * @return
         */
        QString transcriptionClipsOutputDir() const;

        /**
         * @brief setTranscriptionClipsOutputDir
         * @param newTranscriptionClipsOutputDir
         */
        void setTranscriptionClipsOutputDir(const QString &newTranscriptionClipsOutputDir);

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

        AudioTranscriptionSegmentationWorker * m_worker;

        qreal m_clipSec;

        qint32 m_lineLength;

        Url m_transcriptionManifest;

        QString m_transcriptionClipsOutputDir;

};

Q_DECLARE_METATYPE(AudioTranscriptionSegmentationController *)
Q_DECLARE_OPAQUE_POINTER(AudioTranscriptionSegmentationController *)

#endif // AUDIOTRANSCRIPTIONSEGMENTATIONCONTROLLER_H
