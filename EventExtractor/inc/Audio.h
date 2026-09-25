#ifndef AUDIO_H
#define AUDIO_H

#include <QJsonObject>
#include <QString>
#include <QMetaType>

#include <QExplicitlySharedDataPointer>

#include "UniqueId.h"
using namespace utils;

class AudioData;

class Audio
{
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        Q_PROPERTY(UniqueId id READ id WRITE setId)
        Q_PROPERTY(UniqueId eventId READ eventId WRITE setEventId)
        Q_PROPERTY(QString language READ language WRITE setLanguage)
        Q_PROPERTY(QJsonObject annotation READ annotation WRITE setAnnotation)
        Q_PROPERTY(QString transcription READ transcription WRITE setTranscription)
        Q_PROPERTY(QByteArray fileData READ fileData WRITE setFileData)

    public:

        /**
         * @brief Audio
         */
        Audio();

        /**
         * @brief Audio
         * @param newId
         * @param fullAudioId
         * @param newEvtId
         * @param newLang
         * @param newAnnotation
         * @param newTranscription
         * @param data
         * @param path
         */
        Audio(const UniqueId &newId,
              const UniqueId &fullAudioId,
              const UniqueId &newEvtId,
              const QString &newLang,
              const QJsonObject &newAnnotation,
              const QString &newTranscription,
              const QByteArray &data,
              const QString &path);

        /**
         * @brief ~Audio
         */
        virtual ~Audio();

        /**
         * @brief Audio
         * @param right
         */
        Audio(const Audio &right);

        /**
         * @brief operator =
         * @param right
         * @return
         */
        Audio &operator=(const Audio &right);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const Audio &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const Audio &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const Audio *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const Audio *ptr) const;

        /**
         * Typedef inside the class for clean namespacing
         */
        using AudioList_t = QList<Audio>;

        /**
         * @brief id
         * @return
         */
        UniqueId id() const;

        /**
         * @brief setId
         * @param newId
         */
        void setId(const UniqueId &newId);

        /**
         * @brief fullVideoId
         * @return
         */
        UniqueId fullAudioId() const;

        /**
         * @brief setFullVideoId
         * @param newVId
         */
        void setFullAudioId(const UniqueId &newAId);

        /**
         * @brief segmentIndex
         * @return
         */
        qint32 segmentIndex() const;

        /**
         * @brief setSegmentIndex
         * @param newSegmentIndex
         */
        void setSegmentIndex(qint32 newSegmentIndex);

        /**
         * @brief eventId
         * @return
         */
        UniqueId eventId() const;

        /**
         * @brief setEventId
         * @param newEvtId
         */
        void setEventId(const UniqueId &newEvtId);

        /**
         * @brief startTime
         * @return
         */
        double startTime() const;

        /**
         * @brief setStartTime
         * @param newStartTime
         */
        void setStartTime(double newStartTime);

        /**
         * @brief endTime
         * @return
         */
        double endTime() const;

        /**
         * @brief duration
         * @return
         */
        double duration() const;

        /**
         * @brief setTiming
         * @param start
         * @param duration
         */
        void setTiming(double newStart, double newEnd);

        /**
         * @brief state
         * @return
         */
        QString state() const;

        /**
         * @brief setState
         * @param state
         */
        void setState(const QString & newState);

        /**
         * @brief fileData
         * @return
         */
        QByteArray fileData() const;

        /**
         * @brief setFileData
         * @param data
         */
        void setFileData(const QByteArray &data);

        /**
         * @brief filePath
         * @return
         */
        QString filePath() const;

        /**
         * @brief setFilePath
         * @param path
         */
        void setFilePath(const QString & path);

        /**
         * @brief language
         * @return
         */
        QString language() const;

        /**
         * @brief setLanguage
         * @param newLang
         */
        void setLanguage(const QString &newLang);

        /**
         * @brief annotation
         * @return
         */
        QJsonObject annotation() const;

        /**
         * @brief setAnnotation
         * @param newAnnotation
         */
        void setAnnotation(const QJsonObject &newAnnotation);

        /**
         * @brief transcription
         * @return
         */
        QString transcription() const;

        /**
         * @brief setTranscription
         * @param newTranscription
         */
        void setTranscription(const QString &newTranscription);

        /**
         * @brief transcriptionStartTimeSec
         * @return
         */
        qint32 transcriptionStartTimeSec() const;

        /**
         * @brief transcriptionEndTimeSec
         * @return
         */
        qint32 transcriptionEndTimeSec() const;

        /**
         * @brief setTranscriptionTimeSec
         * @param newTranscriptionStartTimeSec
         * @param newTranscriptionEndTimeSec
         */
        void setTranscriptionTimeSec(qint32 newTranscriptionStartTimeSec, qint32 newTranscriptionEndTimeSec);

        /**
         * @brief toJson
         * @return
         */
        QJsonObject toJson() const;

        /**
         * @brief fromJson
         * @param json
         * @return
         */
        static Audio fromJson(const QJsonObject &json);

    private:

        QExplicitlySharedDataPointer<AudioData> sharedDataP;
};

Q_DECLARE_METATYPE(Audio)

Q_DECLARE_METATYPE(Audio::AudioList_t)

#endif // AUDIO_H
