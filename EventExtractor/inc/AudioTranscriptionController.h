#ifndef AUDIOTRANSCRIPTIONCONTROLLER_H
#define AUDIOTRANSCRIPTIONCONTROLLER_H

#include <QObject>
#include <QThread>

#include "Url.h"
using namespace utils;

#include "inc/Defs.h"
#include "inc/PipelineWorker.h"
#include "inc/AudioTranscriptionWorker.h"

class AudioTranscriptionController : public PipelineWorker
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
         * @brief TranscriptionController
         * @param parent
         */
        explicit AudioTranscriptionController(PipelineWorker *parent = nullptr);

        /**
         * @brief ~TranscriptionController
         */
        virtual ~AudioTranscriptionController();

        Q_DISABLE_COPY(AudioTranscriptionController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const AudioTranscriptionController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const AudioTranscriptionController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioTranscriptionController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioTranscriptionController *ptr) const;

        /**
         * @brief model
         * @return
         */
        Deepgram_AIS2TT2SModels_t model() const;

        /**
         * @brief setModel
         * @param newModel
         */
        void setModel(Deepgram_AIS2TT2SModels_t newModel);

        /**
         * @brief audioAnnotationPath
         * @return
         */
        Url audioAnnotationPath() const;

        /**
         * @brief setAudioAnnotationPath
         * @param newAudioAnnotationPath
         */
        void setAudioAnnotationPath(const Url &newAudioAnnotationPath);

        /**
         * @brief setAudioAnnotationPath
         * @param newAudioAnnotationPath
         */
        void setAudioAnnotationPath(const QString &newAudioAnnotationPath);

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
         * @brief annotationDir
         * @return
         */
        const QString & annotationDir() const;

        /**
         * @brief setAnnotationDir
         * @param newAnnotationDir
         */
        void setAnnotationDir(const QString &newAnnotationDir);

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
        void cancelTranscription();

        /**
         * @brief handleInput
         * @param info
         */
        void handleInput(FileInfo info) Q_DECL_OVERRIDE;

    private slots:

        /**
         * @brief handleFinished
         */
        void handleFinished();

        /**
         * @brief handleWorkerError
         * @param error
         */
        void handleWorkerError(QString msg);

    private:

        /**
         * @brief segmentJsonContent
         * @param deepgramJsonPath
         * @param outputDir
         * @param clipSec
         */
        void segmentJsonContent(const QString &deepgramJsonPath,
                                const QString &outputDir,
                                qint32 clipSec = *Constants::Q_QGS_DEFAULT_CLIP_SEC); // Default is 5 min

        /**
         * @brief generateJsonManifest
         * @param outputDir
         * @param clipSec
         */
        void generateJsonManifest(const QString &outputDir, qint32 clipSec);

    private:

        QThread * m_workerThread;
        AudioTranscriptionWorker * m_worker;

        Deepgram_AIS2TT2SModels_t m_model;

        Url m_audioAnnotationPath;

        QString m_annotationDir;

        QString m_annotationClipsOutputDir;

        qreal m_clipSec;
};

Q_DECLARE_METATYPE(AudioTranscriptionController *)
Q_DECLARE_OPAQUE_POINTER(AudioTranscriptionController *)

#endif // AUDIOTRANSCRIPTIONCONTROLLER_H
