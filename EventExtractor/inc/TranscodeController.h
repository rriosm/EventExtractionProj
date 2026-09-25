#ifndef TRANSCODECONTROLLER_H
#define TRANSCODECONTROLLER_H

#include <QObject>
#include <QThread>

#include "Url.h"
using namespace utils;

#include "inc/Defs.h"
#include "inc/PipelineWorker.h"
#include "inc/TranscodeWorker.h"

class TranscodeController : public PipelineWorker
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

    public:

        /**
         * @brief TranscodeController
         * @param parent
         */
        explicit TranscodeController(PipelineWorker *parent = nullptr);

        /**
         * @brief ~TranscodeController
         */
        virtual ~TranscodeController();

        Q_DISABLE_COPY(TranscodeController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const TranscodeController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const TranscodeController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const TranscodeController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const TranscodeController *ptr) const;

        /**
         * @brief setupTranscoding
         * @param inMediaUrl
         * @param outMedia
         */
        void setupAudioTranscoding(const Url &newInMediaUrl, const Url & newOutMedia);

        /**
         * @brief inMediaUrl
         * @return
         */
        const Url &inMediaUrl() const;

        /**
         * @brief setInMediaUrl
         * @param newInMediaUrl
         */
        void setInMediaUrl(const Url &newInMediaUrl);

        /**
         * @brief outMediaUrl
         * @return
         */
        const Url &outMediaUrl() const;

        /**
         * @brief setOutMediaUrl
         * @param newOutMediaUrl
         */
        void setOutMediaUrl(const Url &newOutMediaUrl);

        /**
         * @brief currentProgress
         * @return
         */
        double currentProgress() const;

    signals:

        /**
         * @brief error
         * @param msg
         */
        void error(QString errorMsg);

        /**
         * @brief progressUpdated
         * @param percent
         */
        void progressUpdated(double percent);

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
         * @brief stop
         */
        void cancelTranscoding();

        /**
         * @brief handleInput
         * @param info
         */
        void handleInput(FileInfo info) Q_DECL_OVERRIDE;

        /**
         * @brief onWorkerProgress
         * @param percent
         */
        void onWorkerProgress(double percent);

        /**
         * @brief startProcessing
         */
        void startTranscoding();

        /**
         * @brief setStatus
         * @param status
         */
        void setStatus(const QString &newStatus);

    private slots:

        /**
         * @brief handleFinished
         */
        void handleFinished();

        /**
         * @brief handleWorkerError
         * @param error
         */
        void handleWorkerError(QString msgError);

    private:

        QThread * m_workerThread;
        TranscodeWorker * m_worker;

        double m_currentProgress = 0.0;

        QString m_status;

        Url m_inMediaUrl;
        Url m_outMediaUrl;
};

#endif // TRANSCODECONTROLLER_H
