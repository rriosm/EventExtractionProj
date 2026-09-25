#ifndef CONVERTERWORKER_H
#define CONVERTERWORKER_H

#include <QObject>

#include "Url.h"
using namespace utils;

#include "Defs.h"

class ConverterWorker : public QObject
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
        explicit ConverterWorker(QObject *parent = nullptr);

        /**
         * @brief ~TranscriptionController
         */
        virtual ~ConverterWorker();

        Q_DISABLE_COPY(ConverterWorker)

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator==(const ConverterWorker &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator!=(const ConverterWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const ConverterWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const ConverterWorker *ptr) const;

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
         * @brief transcriptionFileUrl
         * @return
         */
        Url transcriptionFileUrl() const;

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
         * @brief converterType
         * @return
         */
        ConverterType_t converterType() const;

        /**
         * @brief setConverterType
         * @param newConverterType
         */
        void setConverterType(ConverterType_t newConverterType);

        /**
         * @brief convertToTranscript
         * @param lineLength
         * @return
         */
        qint32 convertToTranscript(const qint32 lineLength);

    signals:

        /**
         * @brief error
         * @param message
         */
        void error(const QString &message);

        /**
         * @brief finished
         * @param exitCode
         * @param status
         */
        void finished(qint32 exitCode, qint32 status);

        /**
          * @brief progressUpdated
          * @param percentage
          */
         void progressUpdated(double percentage);

    public slots:

        /**
         * @brief abort
         */
        void abort();

        /**
         * @brief stop
         */
        void stop();

    private:

        /**
         * @brief deepgramConvertToWebVtt
         * @param deepgramData
         * @param lineLength
         * @return
         */
        static QString deepgramConvertToWebVtt(const QJsonObject &deepgramData, qint32 lineLength);

        /**
         * @brief deepgramConvertToSrt
         * @param deepgramData
         * @param lineLength
         * @return
         */
        static QString deepgramConvertToSrt(const QJsonObject &deepgramData, qint32 lineLength);

        /**
         * @brief formatTimestamp
         * @param seconds
         * @return
         */
        static QString formatTimestamp(double seconds);

    private:

        Url m_transcriptionFileUrl;

        Url m_annotationFileUrl;

        ConverterType_t m_converterType;
};

Q_DECLARE_METATYPE(ConverterWorker *)
Q_DECLARE_OPAQUE_POINTER(ConverterWorker *)

#endif // CONVERTERWORKER_H
