#ifndef CONVERTERCONTROLLER_H
#define CONVERTERCONTROLLER_H

#include <QObject>
#include <QThread>

#include "inc/PipelineWorker.h"
#include "inc/ConverterWorker.h"

class ConverterController : public PipelineWorker
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
         * @brief ConverterController
         * @param parent
         */
        explicit ConverterController(PipelineWorker *parent = nullptr);

        /**
         * @brief ~ConverterController
         */
        virtual ~ConverterController();

        Q_DISABLE_COPY(ConverterController)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const ConverterController &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const ConverterController &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const ConverterController *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const ConverterController *ptr) const;

        /**
         * @brief annotationFileUrl
         * @return
         */
        Url annotationFileUrl() const;

        /**
         * @brief setAnnotationFileUrl
         * @param newAnnotationFileUrl
         */
        void setAnnotationFileUrl(const Url &newAnnotationFileUrl);

        /**
         * @brief setAnnotationFileUrl
         * @param newAnnotationFileUrl
         */
        void setAnnotationFileUrl(const QString &newAnnotationFileUrl);

        /**
         * @brief outputFilePath
         * @return
         */
        const Url & transcriptionFileUrl() const;

        /**
         * @brief setTranscriptionFileUrl
         * @param newTranscriptionFileUrl
         */
        void setTranscriptionFileUrl(const Url &newTranscriptionFileUrl);

        /**
         * @brief setTranscriptionFileUrl
         * @param newTranscriptionFileUrl
         */
        void setTranscriptionFileUrl(const QString &newTranscriptionFileUrl);

        /**
         * @brief transcriptionClipsOutputDir
         * @return
         */
        QString transcriptionClipsOutputDir() const;

        /**
         * @brief setTranscriptionClipsOutputDir
         * @param newClipOutputDir
         */
        void setTranscriptionClipsOutputDir(const QString &newClipOutputDir);

        /**
         * @brief converterType
         * @return
         */
        ConverterType_t converterType() const;

        /**
         * @brief setConverterType
         * @param newCvtType
         */
        void setConverterType(ConverterType_t newCvtType);

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
         * @brief clipSec
         * @return
         */
        qreal clipSec() const;

        /**
         * @brief setClipSec
         * @param newClipSec
         */
        void setClipSec(qreal newClipSec);

    signals:

        /**
         * @brief conversionError
         * @param error
         */
        void conversionError(QString error);

        /**
         * @brief progressChanged
         * @param percentage
         */
        void progressChanged(qint32 percentage);

        /**
         * @brief statusMessage
         * @param message
         */
        void statusMessage(QString message);

    public slots:

        /**
         * @brief cancelConversion
         */
        void cancelConversion();

        /**
         * @brief handleInput
         * @param info
         */
        virtual void handleInput(FileInfo info) Q_DECL_OVERRIDE;

    private slots:

        /**
         * @brief handleWorkerError
         * @param error
         */
        void handleWorkerError(QString error);

        /**
         * @brief handleFinished
         */
        void handleFinished();

    private:

        /**
         * @brief segmentVttContent
         * @param vttContentPath
         * @param outputDir
         * @param clipSec
         */
        void segmentVttContent(const QString &vttContentPath,
                               const QString &outputDir,
                               qreal clipSec = *Constants::Q_QGS_DEFAULT_CLIP_SEC); // Default is 5 min

        /**
         * @brief generateVttManifestToJson
         * @param outputDir
         * @param clipSec
         */
        void generateVttManifestToJson(const QString &outputDir, qint32 clipSec);

        /**
         * @brief formatTimestamp
         * @param seconds
         * @return
         */
        static QString formatTimestamp(double seconds);

        /**
         * @brief parseVttTimestampToSeconds: Helper to convert "HH:mm:ss.zzz" back to double seconds
         * @param ts
         * @return
         */
        double parseVttTimestampToSeconds(const QString &ts);

    private:

        QThread * m_workerThread;

        ConverterWorker * m_worker;

        Url m_annotationFileUrl;

        Url m_transcriptionFileUrl;

        ConverterType_t m_cvtType;

        qint32 m_lineLength;

        qreal m_clipSec;

        QString m_transcriptionClipsOutputDir;
};

Q_DECLARE_METATYPE(ConverterController *)
Q_DECLARE_OPAQUE_POINTER(ConverterController *)

#endif // CONVERTERCONTROLLER_H
