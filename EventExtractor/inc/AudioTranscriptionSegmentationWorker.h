#ifndef AUDIOTRANSCRIPTIONSEGMENTATIONWORKER_H
#define AUDIOTRANSCRIPTIONSEGMENTATIONWORKER_H

#include <QObject>
#include <QMutex>

#include "Url.h"
using namespace utils;

class AudioTranscriptionSegmentationWorker : public QObject
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
         * @brief AudioTranscriptionSegmentationWorker
         * @param parent
         */
        explicit AudioTranscriptionSegmentationWorker(QObject *parent = nullptr);

        /**
         * @brief ~AudioTranscriptionSegmentationWorker
         */
        virtual ~AudioTranscriptionSegmentationWorker();

        Q_DISABLE_COPY(AudioTranscriptionSegmentationWorker);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const AudioTranscriptionSegmentationWorker &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const AudioTranscriptionSegmentationWorker &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const AudioTranscriptionSegmentationWorker *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const AudioTranscriptionSegmentationWorker *ptr) const;

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
         * @brief segmentVttContent
         * @param vttContentPath
         */
        void segmentVttContent(const QString &vttContentPath);

    private:

        /**
         * @brief generateVttManifestToJson
         */
        void generateVttManifestToJson();

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

        qreal m_clipSec;

        qint32 m_lineLength;

        Url m_transcriptionManifest;

        QString m_transcriptionClipsOutputDir;

        mutable QMutex m_mutexClipSec;
        mutable QMutex m_mutexLineLength;
        mutable QMutex m_mutexTranscriptionManifest;
        mutable QMutex m_mutexTranscriptionClipsOutputDir;
};

Q_DECLARE_METATYPE(AudioTranscriptionSegmentationWorker *)
Q_DECLARE_OPAQUE_POINTER(AudioTranscriptionSegmentationWorker *)

#endif // AUDIOTRANSCRIPTIONSEGMENTATIONWORKER_H
