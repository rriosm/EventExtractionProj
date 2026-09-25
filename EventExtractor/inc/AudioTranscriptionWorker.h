#ifndef AUDIOTRANSCRIPTIONWORKER_H
#define AUDIOTRANSCRIPTIONWORKER_H

#include <QObject>

// Conditional Includes
#ifndef USE_LIBCURL
// QNAM Headers
class QNetworkAccessManager;
class QNetworkReply;
#else
// LibCurl Forward Declarations
typedef void CURL;

#ifndef CURL_TYPEOF_CURL_OFF_T
#include <curl/system.h>
#endif
#endif

#include "Url.h"
using namespace utils;

#include "Defs.h"

class AudioTranscriptionWorker : public QObject
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
         * @brief TranscribeAudio
         * @param parent
         */
        explicit AudioTranscriptionWorker(QObject *parent = nullptr);

        /**
         * @brief ~TranscribeAudio
         */
        virtual ~AudioTranscriptionWorker();

        Q_DISABLE_COPY(AudioTranscriptionWorker)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const AudioTranscriptionWorker &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const AudioTranscriptionWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioTranscriptionWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioTranscriptionWorker *ptr) const;

        /**
         * @brief setSpeech2TextModel
         * @param aimodel
         */
        void setSpeech2TextAIModel(const Deepgram_AIS2TT2SModels_t aimodel);

        /**
         * @brief speech2TextModel
         * @return
         */
        Deepgram_AIS2TT2SModels_t speech2TextAIModel() const;

        /**
         * @brief setText2SpeechAIModel
         * @param aimodel
         */
        void setText2SpeechAIModel(const Deepgram_AIS2TT2SModels_t aimodel);

        /**
         * @brief text2SpeechAIModel
         * @return
         */
        Deepgram_AIS2TT2SModels_t text2SpeechAIModel() const;

        /**
         * @brief annotationResultFile
         * @return
         */
        QString annotationResultFile() const;

        /**
         * @brief modelAPIKey
         * @return
         */
        AIModels_t modelAPIKey() const;

        /**
         * @brief setModelAPIKey
         * @param newModelAPIKey
         */
        void setModelAPIKey(AIModels_t newModelAPIKey);

        /**
         * @brief outputDir
         * @return
         */
        const QString outputDir() const;

        /**
         * @brief setOutputDir
         * @param newOutputDir
         */
        void setOutputDir(const QString &newOutputDir);

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
        void finished();

    public slots:

        /**
         * @brief processSpeech2Text
         * @param audioFile
         */
        void processSpeech2Text(const utils::Url audioFile);

        /**
         * @brief processText2Speech
         * @param text
         */
        void processText2Speech(const QString &text);

    private slots:

#ifndef USE_LIBCURL
        // Internal slots for QNAM responses

        /**
         * @brief handleS2TResponse
         * @param reply
         * @param destPath
         */
        void handleS2TResponse(QNetworkReply *reply, const QString &destPath);

        /**
         * @brief handleT2SResponse
         * @param reply
         * @param destFile
         */
        void handleT2SResponse(QNetworkReply *reply, std::shared_ptr<QFile> destFile);
#endif

    private:

        // Helper logic to get URLs based on Enum

        /**
         * @brief getModelUrl
         * @param model
         * @return
         */
        QString getModelUrl(Deepgram_AIS2TT2SModels_t model) const;

#ifdef USE_LIBCURL
        /**
         * @brief write_TranscribeAudioResponse_callback
         * @param ptr
         * @param size
         * @param nmemb
         * @param stream
         * @return
         */
        static size_t write_TranscribeAudioResponse_callback(void *ptr, size_t size, size_t nmemb, void *stream);

        /**
         * @brief progress_callback
         * @param clientp
         * @param dltotal
         * @param dlnow
         * @param ultotal
         * @param ulnow
         * @return
         */
        static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);

#endif

    private: // Data members

        Deepgram_AIS2TT2SModels_t m_s2tModel;
        Deepgram_AIS2TT2SModels_t m_t2sModel;
        AIModels_t m_modelAPIKey;
        QString m_annotationFile;
        QString m_outputDir;

#ifndef USE_LIBCURL
        // QNAM Specific Members
        QNetworkAccessManager *m_manager;
#endif

};

Q_DECLARE_METATYPE(AudioTranscriptionWorker *)
Q_DECLARE_OPAQUE_POINTER(AudioTranscriptionWorker *)

#endif // AUDIOTRANSCRIPTIONWORKER_H
