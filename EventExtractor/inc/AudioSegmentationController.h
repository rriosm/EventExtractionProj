#ifndef AUDIOSEGMENTATIONCONTROLLER_H
#define AUDIOSEGMENTATIONCONTROLLER_H

#include <QObject>
#include <QThread>
#include <QElapsedTimer>

#include "Url.h"
using namespace utils;

// Assuming you have an AudioSegmentModel analogous to VideoSegmentModel
#include "inc/PipelineWorker.h"
#include "inc/Audio.h"
#include "inc/AudioModel.h"
#include "inc/IAudioSegmentationWorker.h"

class AudioSegmentationController : public PipelineWorker
{
        Q_OBJECT

        // Property for QML or Property Animation integration
        Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Controller for managing asynchronous MP3 audio segmentation")

        static const quint16 classVersion = quint16(101);

    public:

        explicit AudioSegmentationController(PipelineWorker *parent = nullptr);

        /**
             * @brief AudioSegmentationController
             * @param input
             * @param dir
             * @param clipSec
             * @param parent
             */
        AudioSegmentationController(const QString & input,
                                    const QString & dir,
                                    const qint32& clipSec,
                                    PipelineWorker *parent = nullptr);

        Q_DISABLE_COPY(AudioSegmentationController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const AudioSegmentationController & right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const AudioSegmentationController & right) const;

        /**
         * @brief ~AudioSegmentationController
         */
        virtual ~AudioSegmentationController();

        // --- Configuration Getters/Setters ---

        /**
         * @brief inputAudio
         * @return
         */
        Url inputAudio() const;

        /**
         * @brief setInputAudio
         * @param newInputAudio
         * @return
         */
        bool setInputAudio(const Url &newInputAudio);

        /**
         * @brief setInputAudio
         * @param newInputAudio
         * @return
         */
        bool setInputAudio(const QString &newInputAudio);

        /**
         * @brief outputDir
         * @return
         */
        Url outputDir() const;

        /**
         * @brief setOutputDir
         * @param dir
         * @return
         */
        bool setOutputDir(const Url &dir);

        /**
         * @brief setOutputDir
         * @param dir
         * @return
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
         * @return
         */
        bool setClipSec(const qreal clipSec);

        // --- Model Access ---

        /**
         * @brief audioModel
         * @return
         */
        AudioModel* audioModel() const;

        /**
         * @brief audioModelClone
         * @return
         */
        QSharedPointer<AudioModel> audioModelClone() const;

        // --- Status ---

        /**
         * @brief status
         * @return
         */
        QString status() const;

        /**
         * @brief setupAudioCutter
         * @param inputAudio
         * @param segment_dir
         * @param clipSec
         */
        void setupAudioCutter(const QString & inputAudio,
                              const QString & segment_dir,
                              const qint32 & clipSec);

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
         * @param newStatus
         */
        void statusChanged(const QString &newStatus);

    public slots:

        /**
         * @brief clearResults
         */
        void clearResults();

        /**
         * @brief cancelSegmentation
         */
        void cancelSegmentation();

        /**
         * @brief setStatus
         * @param status
         */
        void setStatus(const QString &status);

        /**
         * @brief handleInput
         * @param info
         */
        virtual void handleInput(FileInfo info) Q_DECL_OVERRIDE;

    private slots:

        /**
         * @brief handleFinished
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
         * @brief exportAudioSegmentsToJson
         * @param results
         * @param outputPath
         * @param sourceAudioName
         * @return
         */
        static bool exportAudioSegmentsToJson(const Audio::AudioList_t &results,
                                              const QString &outputPath,
                                              const QString &sourceAudioName);

        /**
         * @brief getSegmentsMetadata
         * @param segment_dir
         * @return
         */
        Audio::AudioList_t getSegmentsMetadata(const Url & segment_dir);

        /**
         * @brief updateView
         */
        void updateView();

    private:

        QThread * m_workerThread;
        IAudioSegmentationWorker * m_worker;

        Url m_inputAudio;
        Url m_outputDir;
        qreal m_clipSec;
        QString m_status;

        QSharedPointer<AudioModel> m_audioModel;
        qint64 m_lastProcessingDurationMs = 0;
        QElapsedTimer m_timer;
};

Q_DECLARE_METATYPE(AudioSegmentationController *)
Q_DECLARE_OPAQUE_POINTER(AudioSegmentationController *)

#endif // AUDIOSEGMENTATIONCONTROLLER_H
