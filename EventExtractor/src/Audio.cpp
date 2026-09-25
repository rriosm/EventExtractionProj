
#include <QSharedData>
#include <QJsonObject>

#include "inc/ExtractorUtils.h"

#include "inc/Audio.h"

class AudioData : public QSharedData
{
    public:

        AudioData() = default;

        ~AudioData() = default;

        AudioData(const AudioData &other) :
            QSharedData(other),
            m_id(other.m_id),
            m_fullAudioId(other.m_fullAudioId),
            m_segmentIndex(other.m_segmentIndex),
            m_startTime(other.m_startTime),
            m_endTime(other.m_endTime),
            m_duration(other.m_duration),
            m_state(other.m_state),
            m_eventId(other.m_eventId),
            m_language(other.m_language),
            m_annotation(other.m_annotation),
            m_transcription(other.m_transcription),
            m_transcriptionStartTimeSec(other.m_transcriptionStartTimeSec),
            m_transcriptionEndTimeSec(other.m_transcriptionEndTimeSec),
            m_fileData(other.m_fileData),
            m_filePath(other.m_filePath)
        {
            // TODO
        }

        bool operator==(const AudioData &other) const
        {
            return ( (this->m_id == other.m_id) &&
                     (this->m_fullAudioId == other.m_fullAudioId) &&
                     (this->m_segmentIndex == other.m_segmentIndex) &&
                     qFuzzyCompare(this->m_startTime, other.m_startTime) && // Safe double compare
                     qFuzzyCompare(this->m_endTime, other.m_endTime) && // Safe double compare
                     qFuzzyCompare(this->m_duration, other.m_duration) &&
                     (this->m_state == other.m_state) &&
                     (this->m_eventId == other.m_eventId) &&
                     (this->m_language == other.m_language) &&
                     (this->m_annotation == other.m_annotation) &&
                     (this->m_transcription == other.m_transcription) &&
                     (this->m_transcriptionStartTimeSec == other.m_transcriptionStartTimeSec) &&
                     (this->m_transcriptionEndTimeSec == other.m_transcriptionEndTimeSec) &&
                     (this->m_fileData == other.m_fileData) &&
                     (this->m_filePath == other.m_filePath) );
        }

        void updateDuration()
        {
            this->m_duration = (this->m_endTime >= this->m_startTime) ? (this->m_endTime - this->m_startTime) : 0.0;
        }

        UniqueId m_id;
        UniqueId m_fullAudioId;
        qint32 m_segmentIndex;
        double m_startTime;
        double m_endTime;
        double m_duration;
        QString m_state;
        UniqueId m_eventId;
        QString m_language;
        QJsonObject m_annotation;
        QString m_transcription;
        qint32 m_transcriptionStartTimeSec;
        qint32 m_transcriptionEndTimeSec;
        QByteArray m_fileData;
        QString m_filePath;
};

void _updateInternalId(AudioData* data)
{
    if (!data->m_id.isEmpty()) return;

    if (data->m_fileData.isEmpty()) return;
    {
        QByteArray videoSeed = ExtractorUtils::generateContentSeed(data->m_fileData, QCryptographicHash::Md4);

        data->m_id = UniqueId(videoSeed);
    }
}

Audio::Audio() :
    sharedDataP(new AudioData)
{
    // TODO
}

Audio::Audio(const UniqueId &newId,
             const UniqueId &fullAudioId,
             const UniqueId &newEvtId,
             const QString &newLang,
             const QJsonObject &newAnnotation,
             const QString &newTranscription,
             const QByteArray &newData,
             const QString &path) :
    sharedDataP(new AudioData)
{
    this->sharedDataP->m_id = newId;
    this->sharedDataP->m_fullAudioId = fullAudioId;
    this->sharedDataP->m_eventId = newEvtId;
    this->sharedDataP->m_language = newLang;
    this->sharedDataP->m_annotation = newAnnotation;
    this->sharedDataP->m_transcription = newTranscription;
    this->sharedDataP->m_fileData = newData;
    this->sharedDataP->m_filePath = path;
}

Audio::Audio(const Audio &right) :
    sharedDataP(right.sharedDataP)
{
    // TODO
}

Audio::~Audio() = default;

Audio &Audio::operator=(const Audio &right)
{
    if (this != &right)
    {
        this->sharedDataP = right.sharedDataP;
    }

    return *this;
}

bool Audio::operator ==(const Audio & right) const
{
    if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *right.sharedDataP);
}

bool Audio::operator !=(const Audio & right) const
{
    return !(*this == right);
}

bool Audio::operator==(const Audio *ptr) const
{
    return (this == ptr);
}

bool Audio::operator!=(const Audio *ptr) const
{
    return (this != ptr);
}

UniqueId Audio::id() const
{
    return this->sharedDataP->m_id;
}

void Audio::setId(const UniqueId &newId)
{
    if (newId == this->sharedDataP->m_id) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_id = newId;
}

UniqueId Audio::fullAudioId() const
{
    return this->sharedDataP->m_fullAudioId;
}

void Audio::setFullAudioId(const UniqueId &newAId)
{
    if (this->sharedDataP->m_fullAudioId == newAId) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_fullAudioId = newAId;
}

qint32 Audio::segmentIndex() const
{
    return this->sharedDataP->m_segmentIndex;
}

void Audio::setSegmentIndex(qint32 newSegmentIndex)
{
    if (newSegmentIndex == this->sharedDataP->m_segmentIndex) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_segmentIndex = newSegmentIndex;
}

UniqueId Audio::eventId() const
{
    return this->sharedDataP->m_eventId;
}

void Audio::setEventId(const UniqueId &newEvtId)
{
    if (newEvtId == this->sharedDataP->m_eventId) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_eventId = newEvtId;
}

double Audio::startTime() const
{
    return this->sharedDataP->m_startTime;
}

void Audio::setStartTime(double newStartTime)
{
    if ( qFuzzyCompare(this->sharedDataP->m_startTime, newStartTime) ) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_startTime = newStartTime;

    // Maintain the "triad" (Start, End, Duration)
    // Option A: Keep Duration fixed, move End
    // this->sharedDataP->m_endTime = this->sharedDataP->m_startTime + this->sharedDataP->m_duration;

    // OR Option B: Keep End fixed, recalculate Duration
    this->sharedDataP->updateDuration();
}

double Audio::endTime() const
{
    return this->sharedDataP->m_endTime;
}

double Audio::duration() const
{
    return this->sharedDataP->m_duration;
}

void Audio::setTiming(double newStart, double newEnd)
{
    if (qFuzzyCompare(this->sharedDataP->m_startTime, newStart) &&
        qFuzzyCompare(this->sharedDataP->m_endTime, newEnd) ) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_startTime = newStart;
    this->sharedDataP->m_endTime = newEnd;

    this->sharedDataP->updateDuration();
}

QString Audio::state() const
{
    return this->sharedDataP->m_state;
}

void Audio::setState(const QString & newState)
{
    if (this->sharedDataP->m_state == newState) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_state = newState;
}

QByteArray Audio::fileData() const
{
    return this->sharedDataP->m_fileData;
}

void Audio::setFileData(const QByteArray &newData)
{
    if (newData == this->sharedDataP->m_fileData) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_fileData = newData;

    _updateInternalId(this->sharedDataP.data());
}

QString Audio::filePath() const
{
    return this->sharedDataP->m_filePath;
}

void Audio::setFilePath(const QString & newPath)
{
    if (this->sharedDataP->m_filePath == newPath) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_filePath = newPath;
}

QString Audio::language() const
{
    return this->sharedDataP->m_language;
}

void Audio::setLanguage(const QString &newLang)
{
    if (newLang == this->sharedDataP->m_language) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_language = newLang;
}

QJsonObject Audio::annotation() const
{
    return this->sharedDataP->m_annotation;
}

void Audio::setAnnotation(const QJsonObject &newAnnotation)
{
    if (newAnnotation == this->sharedDataP->m_annotation) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_annotation = newAnnotation;
}

QString Audio::transcription() const
{
    return this->sharedDataP->m_transcription;
}

void Audio::setTranscription(const QString &newTranscription)
{
    if (newTranscription == this->sharedDataP->m_transcription) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_transcription = newTranscription;
}

qint32 Audio::transcriptionStartTimeSec() const
{
    return this->sharedDataP->m_transcriptionStartTimeSec;
}

qint32 Audio::transcriptionEndTimeSec() const
{
    return this->sharedDataP->m_transcriptionEndTimeSec;
}

void Audio::setTranscriptionTimeSec(qint32 newTranscriptionStartTimeSec, qint32 newTranscriptionEndTimeSec)
{
    if ((newTranscriptionStartTimeSec < 0) ||
        (newTranscriptionEndTimeSec < 0) ||
        (newTranscriptionStartTimeSec > newTranscriptionEndTimeSec))
    {
        qWarning() << Q_FUNC_INFO << "Invalid timing parameters passed:"
                   << newTranscriptionStartTimeSec << "->" << newTranscriptionEndTimeSec;
        return;
    }

    if (this->sharedDataP->m_transcriptionStartTimeSec == newTranscriptionStartTimeSec &&
        this->sharedDataP->m_transcriptionEndTimeSec == newTranscriptionEndTimeSec) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_transcriptionStartTimeSec = newTranscriptionStartTimeSec;
    this->sharedDataP->m_transcriptionEndTimeSec = newTranscriptionEndTimeSec;
}

QJsonObject Audio::toJson() const
{
    QJsonObject obj;

    // --- Identifiers ---
    // Removed the QString::fromUtf8 wrapper since toHex() already returns a QString
    if (!this->sharedDataP->m_id.isEmpty()) {
        // obj[QStringLiteral("audio_id")] = this->sharedDataP->m_id.toHex();
        obj.insert(QLatin1String("audio_id"), this->sharedDataP->m_id.toHex());
    }

    if (!this->sharedDataP->m_eventId.isEmpty()) {
        // obj[QStringLiteral("event_id")] = this->sharedDataP->m_eventId.toHex();
        obj.insert(QLatin1String("event_id"), this->sharedDataP->m_eventId.toHex());
    }

    if (!this->sharedDataP->m_fullAudioId.isEmpty()) {
        // obj[QStringLiteral("full_audio_id")] = this->sharedDataP->m_fullAudioId.toHex();
        obj.insert(QLatin1String("full_audio_id"), this->sharedDataP->m_fullAudioId.toHex());
    }

    // obj[QStringLiteral("segment_index")] = static_cast<double>(this->sharedDataP->m_segmentIndex);
    // Direct integer assignment (QJsonValue supports int / qint64 directly)
    obj.insert(QLatin1String("segment_index"), this->sharedDataP->m_segmentIndex);

    // --- Timing & State ---
    // obj[QStringLiteral("start_time")] = this->sharedDataP->m_startTime;
    // obj[QStringLiteral("end_time")] = this->sharedDataP->m_endTime;
    // obj[QStringLiteral("duration")] = this->sharedDataP->m_duration;
    obj.insert(QLatin1String("start_time"), this->sharedDataP->m_startTime);
    obj.insert(QLatin1String("end_time"),   this->sharedDataP->m_endTime);
    obj.insert(QLatin1String("duration"),   this->sharedDataP->m_duration);

    if (!this->sharedDataP->m_state.isEmpty()) {
        // obj[QStringLiteral("state")] = this->sharedDataP->m_state;
        obj.insert(QLatin1String("state"), this->sharedDataP->m_state);
    }

    if (!this->sharedDataP->m_filePath.isEmpty()) {
        // obj[QStringLiteral("file_path")] = this->sharedDataP->m_filePath;
        obj.insert(QLatin1String("file_path"), this->sharedDataP->m_filePath);
    }

    // --- Metadata ---
    if (!this->sharedDataP->m_language.isEmpty()) {
        // obj[QStringLiteral("language")] = this->sharedDataP->m_language;
        obj.insert(QLatin1String("language"), this->sharedDataP->m_language);
    }

    if (!this->sharedDataP->m_transcription.isEmpty()) {
        // obj[QStringLiteral("transcription")] = this->sharedDataP->m_transcription;
        obj.insert(QLatin1String("transcription"), this->sharedDataP->m_transcription);
    }

    // obj[QStringLiteral("vtt_start_time")] = this->sharedDataP->m_transcriptionStartTimeSec;
    // obj[QStringLiteral("vtt_end_time")]   = this->sharedDataP->m_transcriptionEndTimeSec;

    // Only serialize VTT timings if they are set (assuming >= 0 indicates valid)
    if (this->sharedDataP->m_transcriptionStartTimeSec >= 0) {
        obj.insert(QLatin1String("vtt_start_time"), this->sharedDataP->m_transcriptionStartTimeSec);
    }
    if (this->sharedDataP->m_transcriptionEndTimeSec >= 0) {
        obj.insert(QLatin1String("vtt_end_time"), this->sharedDataP->m_transcriptionEndTimeSec);
    }

    if (!this->sharedDataP->m_annotation.isEmpty()) {
        // obj[QStringLiteral("annotation")] = this->sharedDataP->m_annotation;
        obj.insert(QLatin1String("annotation"), this->sharedDataP->m_annotation);
    }

    // --- Binary Data ---
    if (!this->sharedDataP->m_fileData.isEmpty()) {
        // obj[QStringLiteral("file_data_base64")] = QString::fromLatin1(this->sharedDataP->m_fileData.toBase64());
        obj.insert(QLatin1String("file_data_base64"), QString::fromLatin1(this->sharedDataP->m_fileData.toBase64()));
    }

    return obj;
}

Audio Audio::fromJson(const QJsonObject &json)
{
    Audio audio;

    // Helper lambda to fetch values cleanly with single lookups
    auto getVal = [&json](QLatin1String key) {
        return json.value(key);
    };

    // --- Identifiers ---
    // if (json.contains(QLatin1String("audio_id")) && json[QLatin1String("audio_id")].isString()) {
    //     audio.setId(UniqueId::fromString(json[QLatin1String("audio_id")].toString()));
    // }

    const QJsonValue audioIdVal = getVal(QLatin1String("audio_id"));
    if (audioIdVal.isString()) {
        audio.setId(UniqueId::fromString(audioIdVal.toString()));
    }

    // if (json.contains(QLatin1String("event_id")) && json[QLatin1String("event_id")].isString()) {
    //     audio.setEventId(UniqueId::fromString(json["event_id"].toString()));
    // }

    const QJsonValue eventIdVal = getVal(QLatin1String("event_id"));
    if (eventIdVal.isString()) {
        audio.setEventId(UniqueId::fromString(eventIdVal.toString()));
    }

    // if (json.contains(QLatin1String("full_audio_id")) && json[QLatin1String("full_audio_id")].isString()) {
    //     audio.setFullAudioId(UniqueId::fromString(json[QLatin1String("full_audio_id")].toString()));
    // }

    const QJsonValue fullAudioIdVal = getVal(QLatin1String("full_audio_id"));
    if (fullAudioIdVal.isString()) {
        audio.setFullAudioId(UniqueId::fromString(fullAudioIdVal.toString()));
    }

    // if (json.contains(QStringLiteral("segment_index"))) {
    //     audio.setSegmentIndex(static_cast<qint32>(json[QStringLiteral("segment_index")].toDouble()));
    // }

    const QJsonValue segIdxVal = getVal(QLatin1String("segment_index"));
    if (segIdxVal.isDouble()) {
        audio.setSegmentIndex(segIdxVal.toInt());
    }

    // --- Timing & State ---
    // Using .toDouble(0.0) provides a safe fallback if the keys are missing or invalid
    // double start = json.contains(QLatin1String("start_time")) ? json[QLatin1String("start_time")].toDouble(0.0) : 0.0;
    // double end = json.contains(QLatin1String("end_time")) ? json[QLatin1String("end_time")].toDouble(0.0) : 0.0;
    // audio.setTiming(start, end);

    // --- Timing & State ---
    const QJsonValue startVal = getVal(QLatin1String("start_time"));
    const QJsonValue endVal   = getVal(QLatin1String("end_time"));
    if (startVal.isDouble() || endVal.isDouble()) {
        audio.setTiming(startVal.toDouble(0.0), endVal.toDouble(0.0));
    }

    // qint32 vttStart = json.contains(QLatin1String("vtt_start_time")) ? json[QLatin1String("vtt_start_time")].toInt(0.0) : 0.0;
    // qint32 vttEnd = json.contains(QLatin1String("vtt_end_time")) ? json[QLatin1String("vtt_end_time")].toInt(0.0) : 0.0;
    // audio.setTranscriptionTimeSec(vttStart, vttEnd);

    const QJsonValue vttStartVal = getVal(QLatin1String("vtt_start_time"));
    const QJsonValue vttEndVal   = getVal(QLatin1String("vtt_end_time"));
    if (vttStartVal.isDouble() && vttEndVal.isDouble()) {
        audio.setTranscriptionTimeSec(
            static_cast<qint32>(qRound(vttStartVal.toDouble())),
            static_cast<qint32>(qRound(vttEndVal.toDouble()))
            );
    }

    // if (json.contains(QLatin1String("state")) && json[QLatin1String("state")].isString()) {
    //     audio.setState(json[QLatin1String("state")].toString());
    // }

    const QJsonValue stateVal = getVal(QLatin1String("state"));
    if (stateVal.isString()) {
        audio.setState(stateVal.toString());
    }

    // if (json.contains(QLatin1String("file_path")) && json[QLatin1String("file_path")].isString()) {
    //     audio.setFilePath(json[QLatin1String("file_path")].toString());
    // }

    const QJsonValue filePathVal = getVal(QLatin1String("file_path"));
    if (filePathVal.isString()) {
        audio.setFilePath(filePathVal.toString());
    }

    // --- Metadata ---
    // if (json.contains(QLatin1String("language")) && json[QLatin1String("language")].isString()) {
    //     audio.setLanguage(json["language"].toString());
    // }

    const QJsonValue langVal = getVal(QLatin1String("language"));
    if (langVal.isString()) {
        audio.setLanguage(langVal.toString());
    }

    // if (json.contains(QLatin1String("transcription")) && json[QLatin1String("transcription")].isString()) {
    //     audio.setTranscription(json[QLatin1String("transcription")].toString());
    // }

    const QJsonValue transVal = getVal(QLatin1String("transcription"));
    if (transVal.isString()) {
        audio.setTranscription(transVal.toString());
    }

    // if (json.contains(QLatinString("annotation")) && json[QLatinString("annotation")].isObject()) {
    //     audio.setAnnotation(json["annotation"].toObject());
    // }

    const QJsonValue annotVal = getVal(QLatin1String("annotation"));
    if (annotVal.isObject()) {
        audio.setAnnotation(annotVal.toObject());
    }

    // --- Binary Data ---
    // if (json.contains(QLatin1String("file_data_base64")) && json[QLatin1String("file_data_base64")].isString()) {
    //     QByteArray base64Data = json[QLatin1String("file_data_base64")].toString().toLatin1();
    //     audio.setFileData(QByteArray::fromBase64(base64Data));
    // }

    const QJsonValue fileDataVal = getVal(QLatin1String("file_data_base64"));
    if (fileDataVal.isString()) {
        audio.setFileData(QByteArray::fromBase64(fileDataVal.toString().toLatin1()));
    }

    return audio;
}

static int registerAudioMetatype()
{
    return qRegisterMetaType<Audio>("Audio");
}

static int dummyAudio = registerAudioMetatype();

static int registerAudioListMetatype()
{
    return qRegisterMetaType<Audio::AudioList_t>("Audio::AudioList_t");
}

static int dummyAudioList = registerAudioListMetatype();
