#ifndef EVENTEXTRACTIONCONTROLLER_H
#define EVENTEXTRACTIONCONTROLLER_H

#include <QObject>
#include <QJsonDocument>
#include <QFile>
#include <QThread>

#ifndef USE_LIBCURL
// QNAM Headers
class QNetworkAccessManager;
class QNetworkReply;
#else
// LibCurl Forward Declarations
typedef void CURL;
#endif

#include "inc/Defs.h"

#include "inc/EventExtractionWorker.h"
#include "inc/MessageHistory.h"
#include "inc/PipelineWorker.h"
#include "inc/VideoSegmentInfo.h"
// #include "inc/KeywordSearchEngine.h"
// #include "inc/PromptManager.h"
// #include "inc/UploadManager.h"

class EventExtractionController : public PipelineWorker
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Event Extraction controller")

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief EventExtractionController
         * @param parent
         */
        explicit EventExtractionController(PipelineWorker *parent = nullptr);

        /**
         * @brief ~EventExtractionController
         */
        virtual ~EventExtractionController();

        Q_DISABLE_COPY(EventExtractionController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const EventExtractionController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const EventExtractionController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const EventExtractionController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const EventExtractionController *ptr) const;

        /**
         * @brief setupModel
         * @param newModel
         * @param customModelString
         * @param customAPIUrl
         * @param customAPIKey
         */
        void setupModel(AIModels_t newModel,
                        const QString &customModelString = QString(),
                        const QString &customAPIUrl = QString(),
                        const QString &customAPIKey = QString());

        /**
         * @brief setOutputFile
         * @param newFilePath
         */
        void setOutputFile(const QString &newFilePath);

        // TODO setOutputFilePattern...!!!

        /**
         * @brief setSystemPrompt
         * @param newSysPrompt
         */
        void setSystemPrompt(const QString &newSysPrompt);

        /**
         * @brief addUserMessage
         * @param content
         */
        void addUserMessage(const QString &content);

        /**
         * @brief addUserMessageWithImage
         * @param text
         * @param imagePath
         */
        void addUserMessageWithImage(const QString &text, const QString &imagePath);

        /**
         * @brief addUserMessageWithVideo
         * @param text
         * @param videoPath
         */
        void addUserMessageWithVideo(const QString &text, const QString &videoPath);

        /**
         * @brief addUserMessageWithVideosList
         * @param text
         * @param videosPaths
         */
        void addUserMessageWithVideosList(const QString &text, const QStringList &videosPaths);

        /**
         * @brief addUserMessageWithVideosList
         * @param filteredSegsWithPrompts
         */
        void addUserMessageFilteredSegsWithPrompts(const SegmentsWithPrompts & filteredSegsWithPrompts);

        /**
         * @brief addMessageHistory
         * @param newMsgHistory
         */
        void addMessageHistory(const MessageHistory &newMsgHistory);

        /**
         * @brief addTermForSearching
         * @param term
         */
        void addTermForSearching(const QString &term);

        /**
         * @brief addTermsForSearching
         * @param searchTerms
         */
        void addTermsForSearching(const QStringList &searchTerms);

        /**
         * @brief searchTerms
         * @return
         */
        [[nodiscard]] const QStringList searchTerms() const;

        /**
         * @brief setSearchType
         * @param sType
         */
        void setSearchType(KeywordMatchType_t sType);

        /**
         * @brief searchType
         * @return
         */
        [[nodiscard]] KeywordMatchType_t searchType() const;

        /**
         * @brief clearSearchTerms
         */
        void clearSearchTerms();

        /**
         * @brief minConfidenceThreshold
         * @return
         */
        [[nodiscard]] double minConfidenceThreshold() const;

        /**
         * @brief setMinConfidenceThreshold
         * @param newMinConfidenceThreshold
         */
        void setMinConfidenceThreshold(double newMinConfidenceThreshold);

        /**
         * @brief taskTemplateType
         * @return
         */
        [[nodiscard]] TaskTemplate taskTemplateType() const;

        /**
         * @brief setTaskTemplateType
         * @param newTaskTemplateType
         */
        void setTaskTemplateType(TaskTemplate newTaskTemplateType);

        /**
         * @brief totalIterations The total iterations for the current segment
         * @return
         */
        [[nodiscard]] qint32 totalIterations() const;

        /**
         * @brief setTotalIterations
         * @param newTotalIterations
         */
        void setTotalIterations(qint32 newTotalIterations);

        /**
         * @brief currentIteration The current iteration for the current segment
         * @return
         */
        [[nodiscard]] qint32 currentIteration() const;

        /**
         * @brief setCurrentIteration
         * @param newCurrentIteration
         */
        void setCurrentIteration(qint32 newCurrentIteration);

    signals:

        /**
         * @brief chunkFinished
         * @param success
         */
        void chunkFinished(bool success);

        /**
         * @brief error
         * @param msg
         */
        void error(QString errorMsg);

        /**
         * @brief progressUpdated
         * @param percent
         */
        void progressUpdated(qint32 percent);

        /**
         * @brief requestAbort
         */
        void requestAbort();

        /**
         * @brief statusChanged
         * @param nesStatus
         */
        void statusChanged(const QString &newStatus);

    public slots:

        /**
         * @brief cancelEventExtraction
         */
        void cancelEventExtraction();

        /**
         * @brief handleInput
         * @param info
         */
        void handleInput(FileInfo fInfo) Q_DECL_OVERRIDE;

        /**
         * @brief onChunkFinished
         * @param success
         */
        void onChunkFinished(bool success);

        /**
         * @brief onChunkResponseReceived
         * @param fullSegmentHexId
         * @param response
         */
        void onChunkResponseReceived(const QString &fullSegmentHexId, const QString &response);

        /**
         * @brief processNextChunkNoMapList This function is the first to be implemented.
         *                                   It uses a list of video segments paths and only
         *                                   one default user prompt.
         */
        void processNextChunkNoMapList();

        /**
         * @brief processNextChunk This function uses filteredSegmentsWithPrompts_t
         */
        void processNextChunk();

        /**
         * @brief setStatus
         * @param newStatus
         */
        void setStatus(const QString &newStatus);

        /**
         * @brief startEventExtraction
         */
        void startEventExtraction();

    private slots:

        /**
         * @brief handleFinished
         */
        void handleFinished();

        /**
         * @brief handleWokerError
         * @param errorMsg
         */
        void handleWokerError(QString errorMsg);

    private:

        /**
         * @brief loadJsonFile
         * @param filePath
         * @param jsonDoc
         * @return
         */
        [[nodiscard]] bool loadJsonFile(const QString &filePath, QJsonDocument &jsonDoc);

        /**
         * @brief parseSynchronizedManifest
         * @param manifestPath
         * @return
         */
        [[nodiscard]] VideoSegmentInfo::VideoSegmentInfoList_t parseSynchronizedManifest(FileInfo manifestPath);

        /**
         * @brief prepareFilteredSegsWithPrompts
         * @param filteredSegments
         * @return
         */
        [[nodiscard]] SegmentsWithPrompts prepareFilteredSegsWithPrompts(const VideoSegmentInfo::VideoSegmentInfoList_t & filteredSegments);

        /**
         * @brief formatMinutesSeconds
         * @param totalSeconds
         * @param minuteDigits
         * @return
         */
        [[nodiscard]] static QString formatMinutesSeconds(double totalSeconds, qint32 minuteDigits = 3);

    private:

        QThread * m_eventExtractionWorkerThread;

        EventExtractionWorker * m_eventExtractionWorker;

        QString m_status;

        AIModels_t m_model;
        QString m_customModelString;
        QString m_customAPIUrl;
        QString m_customAPIKey;

        QStringList m_videoQueue;

        QStringList m_searchTerms;
        KeywordMatchType_t m_searchTermsType;

        QString m_lastError;

        double m_minConfidenceThreshold;

        SegmentsWithPrompts m_filteredSegmentsWithPromptsList;

        QString m_systemPromptCache;
        QString m_userPromptCache;

        qint32 m_totalChunks;
        qint32 m_currentChunk;
        QString m_outputFilePath;
        QFile *m_masterFile;

        TaskTemplate m_taskTemplateType;

        // The current iteration run.
        qint32 m_currentIteration;

        // The total iterations to be run.
        qint32 m_totalIterations;
};

Q_DECLARE_METATYPE(EventExtractionController *)
Q_DECLARE_OPAQUE_POINTER(EventExtractionController *)

#endif // EVENTEXTRACTIONCONTROLLER_H
