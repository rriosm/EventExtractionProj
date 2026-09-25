
#include <QSharedData>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QDebug>
#include <QTextStream>

// #include <cmath> // For fuzzy double comparison

// 2. Your Project Headers come LAST
#include "inc/VideoSegmentInfo.h"
#include "inc/Audio.h"
#include "inc/ExtractorUtils.h"

class VideoSegmentInfoData : public QSharedData
{
    public:
        VideoSegmentInfoData() :
            m_segmentIndex(-1),
            m_startTime(0.0),
            m_endTime(0.0),
            m_duration(0.0),
            m_slowdownFactor(1),
            m_vttAccuracy(0.0),
            m_totalEvents(0)
        {
            // TODO
        }

        ~VideoSegmentInfoData() = default;

        VideoSegmentInfoData(const VideoSegmentInfoData & videoSegmentInfoData) :
            QSharedData(),
            m_id(videoSegmentInfoData.m_id),
            m_fullVideoId(videoSegmentInfoData.m_fullVideoId),
            m_segmentIndex(videoSegmentInfoData.m_segmentIndex),
            m_startTime(videoSegmentInfoData.m_startTime),
            m_endTime(videoSegmentInfoData.m_endTime),
            m_duration(videoSegmentInfoData.m_duration),
            m_state(videoSegmentInfoData.m_state),
            m_filePath(videoSegmentInfoData.m_filePath),
            m_audioTrack(videoSegmentInfoData.m_audioTrack),
            m_slowdownFactor(videoSegmentInfoData.m_slowdownFactor),
            m_priorityEvents(videoSegmentInfoData.m_priorityEvents),
            m_vttAccuracy(videoSegmentInfoData.m_vttAccuracy),
            m_totalEvents(videoSegmentInfoData.m_totalEvents),
            m_analysisDate(videoSegmentInfoData.m_analysisDate),
            m_vttConfirmedLog(videoSegmentInfoData.m_vttConfirmedLog),
            m_vttRejectedLog(videoSegmentInfoData.m_vttRejectedLog)
        {
            // TODO
        }

        bool operator==(const VideoSegmentInfoData &other) const
        {
            return ((this->m_filePath == other.m_filePath) &&
                    (this->m_id == other.m_id) &&
                    (this->m_fullVideoId == other.m_fullVideoId) &&
                    (this->m_segmentIndex == other.m_segmentIndex) &&
                    (qFuzzyCompare(this->m_startTime, other.m_startTime)) && // Safe double compare
                    (qFuzzyCompare(this->m_endTime, other.m_endTime)) && // Safe double compare
                    (qFuzzyCompare(this->m_duration, other.m_duration)) &&
                    (this->m_state == other.m_state) &&
                    (this->m_audioTrack == other.m_audioTrack) &&
                    (this->m_slowdownFactor == other.m_slowdownFactor) &&
                    (qFuzzyCompare(this->m_vttAccuracy, other.m_vttAccuracy)) &&
                    (this->m_analysisDate == other.m_analysisDate) &&
                    (this->m_priorityEvents == other.m_priorityEvents));
        }

        void updateDuration()
        {
            this->m_duration = (this->m_endTime >= this->m_startTime) ? (this->m_endTime - this->m_startTime) : 0.0;
        }

        UniqueId m_id;
        UniqueId m_fullVideoId;
        qint32 m_segmentIndex;
        double m_startTime;
        double m_endTime;
        double m_duration;
        QString m_state;
        QString m_filePath;
        Audio m_audioTrack;

        qreal m_slowdownFactor;          // "slowdown_factor": 1, 2, 3, 4, 5, 6,...
        QStringList m_priorityEvents;    // "priority_events_detected": ["GOAL", ...]
        double m_vttAccuracy;            // "vtt_accuracy": "66.7%" (stored as 0.667)
        int m_totalEvents;               // "total_events": 5
        QString m_analysisDate;          // "analysis_date": "2026-03-26"

        // Verification Logs
        QStringList m_vttConfirmedLog;   // From vtt_verification -> confirmed
        QStringList m_vttRejectedLog;    // From vtt_verification -> rejected
};

VideoSegmentInfo::VideoSegmentInfo() :
    sharedDataP(new VideoSegmentInfoData)
{
    // TODO
}

VideoSegmentInfo::VideoSegmentInfo(const QString & path,
                                   const UniqueId & id,
                                   const UniqueId & fullVideoId,
                                   const double start,
                                   const double end,
                                   const double duration,
                                   const QString & state,
                                   const quint32 index) :
    sharedDataP(new VideoSegmentInfoData)
{
    this->sharedDataP->m_id = id;
    this->sharedDataP->m_fullVideoId = fullVideoId;
    this->sharedDataP->m_segmentIndex = index;
    this->sharedDataP->m_startTime = start;
    this->sharedDataP->m_endTime = end;
    this->sharedDataP->m_duration = duration;
    this->sharedDataP->m_state = state;
    this->sharedDataP->m_filePath = path;
}

VideoSegmentInfo::~VideoSegmentInfo()
{
    // TODO
}

VideoSegmentInfo::VideoSegmentInfo(const VideoSegmentInfo & rhs) :
    sharedDataP(rhs.sharedDataP)
{
    // TODO
}

VideoSegmentInfo &VideoSegmentInfo::operator=(const VideoSegmentInfo & rhs)
{
    if (this != &rhs)
    {
        this->sharedDataP.operator=(rhs.sharedDataP);
    }

    return *this;
}

bool VideoSegmentInfo::operator ==(const VideoSegmentInfo & right) const
{
    if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *right.sharedDataP); // Deep value compare
}

bool VideoSegmentInfo::operator !=(const VideoSegmentInfo & right) const
{
    return !(*this == right);
}

bool VideoSegmentInfo::operator==(const VideoSegmentInfo *ptr) const
{
    return (this == ptr);
}

bool VideoSegmentInfo::operator!=(const VideoSegmentInfo *ptr) const
{
    return (this != ptr);
}

UniqueId VideoSegmentInfo::id() const
{
    if (!this->sharedDataP->m_id.isEmpty())
        return this->sharedDataP->m_id;

    if (!this->sharedDataP->m_filePath.isEmpty() &&
        QFileInfo::exists(this->sharedDataP->m_filePath))
    {
        QByteArray contentSeed = ExtractorUtils::generateContentSeed(this->sharedDataP->m_filePath, QCryptographicHash::Md4);

        VideoSegmentInfo *mutableThis = const_cast<VideoSegmentInfo*>(this);
        mutableThis->sharedDataP.detach();

        mutableThis->sharedDataP->m_id.id(contentSeed);
    }

    return this->sharedDataP->m_id;
}

void VideoSegmentInfo::setId(const UniqueId &newId)
{
    if (newId == this->sharedDataP->m_id) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_id = newId;
}

qint32 VideoSegmentInfo::segmentIndex() const
{
    return this->sharedDataP->m_segmentIndex;
}

void VideoSegmentInfo::setSegmentIndex(qint32 newIndex)
{
    if (newIndex == this->sharedDataP->m_segmentIndex) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_segmentIndex = newIndex;
}

UniqueId VideoSegmentInfo::fullVideoId() const
{
    return this->sharedDataP->m_fullVideoId;
}

void VideoSegmentInfo::setFullVideoId(const UniqueId &newVid)
{
    if (this->sharedDataP->m_fullVideoId == newVid) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_fullVideoId = newVid;
}

double VideoSegmentInfo::startTime() const
{
    return this->sharedDataP->m_startTime;
}

void VideoSegmentInfo::setStartTime(const double newStartTime)
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

double VideoSegmentInfo::endTime() const
{
    return this->sharedDataP->m_endTime;
}

double VideoSegmentInfo::duration() const
{
    return this->sharedDataP->m_duration;
}

void VideoSegmentInfo::setTiming(double newStart, double newEnd)
{
    if (qFuzzyCompare(this->sharedDataP->m_startTime, newStart) &&
        qFuzzyCompare(this->sharedDataP->m_endTime, newEnd) ) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_startTime = newStart;
    this->sharedDataP->m_endTime = newEnd;

    this->sharedDataP->updateDuration();
}

QString VideoSegmentInfo::state() const
{
    return this->sharedDataP->m_state;
}

void VideoSegmentInfo::setState(const QString & newState)
{
    if (this->sharedDataP->m_state == newState) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_state = newState;
}

Audio VideoSegmentInfo::audioTrack() const
{
    return this->sharedDataP->m_audioTrack;
}

void VideoSegmentInfo::setAudioTrack(const Audio &newAudio)
{
    if (newAudio == this->sharedDataP->m_audioTrack) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_audioTrack = newAudio;
}

QString VideoSegmentInfo::filePath() const
{
    return this->sharedDataP->m_filePath;
}

void VideoSegmentInfo::setFilePath(const QString & newPath)
{
    if (this->sharedDataP->m_filePath == newPath) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_filePath = newPath;
}

QStringList VideoSegmentInfo::priorityEvents() const
{
    return this->sharedDataP->m_priorityEvents;
}

void VideoSegmentInfo::setPriorityEvents(const QStringList &newPriorityEvents)
{
    if (newPriorityEvents == this->sharedDataP->m_priorityEvents) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_priorityEvents = newPriorityEvents;
}

double VideoSegmentInfo::vttAccuracy() const
{
    return this->sharedDataP->m_vttAccuracy;
}

void VideoSegmentInfo::setVttAccuracy(double newVttAccuracy)
{
    if (newVttAccuracy == this->sharedDataP->m_vttAccuracy) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_vttAccuracy = newVttAccuracy;
}

int VideoSegmentInfo::totalEvents() const
{
    return this->sharedDataP->m_totalEvents;
}

void VideoSegmentInfo::setTotalEvents(qint32 newTotalEvents)
{
    if (newTotalEvents == this->sharedDataP->m_totalEvents) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_totalEvents = newTotalEvents;
}

QString VideoSegmentInfo::analysisDate() const
{
    return this->sharedDataP->m_analysisDate;
}

void VideoSegmentInfo::setAnalysisDate(const QString &newAnalysisDate)
{
    if (newAnalysisDate == this->sharedDataP->m_analysisDate) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_analysisDate = newAnalysisDate;
}

QStringList VideoSegmentInfo::vttConfirmedLog() const
{
    return this->sharedDataP->m_vttConfirmedLog;
}

void VideoSegmentInfo::setVttConfirmedLog(const QStringList &newVttConfirmedLog)
{
    if (newVttConfirmedLog == this->sharedDataP->m_vttConfirmedLog) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_vttConfirmedLog = newVttConfirmedLog;
}

QStringList VideoSegmentInfo::vttRejectedLog() const
{
    return this->sharedDataP->m_vttRejectedLog;
}

void VideoSegmentInfo::setVttRejectedLog(const QStringList &newVttRejectedLog)
{
    if (newVttRejectedLog == this->sharedDataP->m_vttRejectedLog) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_vttRejectedLog = newVttRejectedLog;
}

int VideoSegmentInfo::slowdownFactor() const
{
    return this->sharedDataP->m_slowdownFactor;
}

void VideoSegmentInfo::setSlowdownFactor(qreal newSlowdownFactor)
{
    if (newSlowdownFactor == this->sharedDataP->m_slowdownFactor) return;

    this->sharedDataP.detach();
    this->sharedDataP->m_slowdownFactor = newSlowdownFactor;
}

QJsonObject VideoSegmentInfo::toJson() const
{
    QJsonObject obj;

    // --- Core Segment Data ---
    obj[QStringLiteral("segment_id")]       = this->sharedDataP->m_id.toHex();
    obj[QStringLiteral("segment_index")]    = static_cast<qint32>(this->sharedDataP->m_segmentIndex);
    obj[QStringLiteral("fullvideo_id")]     = this->sharedDataP->m_fullVideoId.toHex();
    obj[QStringLiteral("start_time")]       = this->sharedDataP->m_startTime;
    obj[QStringLiteral("end_time")]         = this->sharedDataP->m_endTime;
    obj[QStringLiteral("duration")]         = this->sharedDataP->m_duration;
    obj[QStringLiteral("state")]            = this->sharedDataP->m_state;
    obj[QStringLiteral("segment_path")]     = this->sharedDataP->m_filePath;
    obj[QStringLiteral("audio_track")]      = this->sharedDataP->m_audioTrack.toJson();

    // --- Clip Info ---
    QJsonObject clipInfo;
    clipInfo[QStringLiteral("slowdown_factor")]          = this->sharedDataP->m_slowdownFactor;
    clipInfo[QStringLiteral("priority_events_detected")] = QJsonArray::fromStringList(this->sharedDataP->m_priorityEvents);

    // Formatting accuracy back to string "XX.X%"
    QString accuracyStr = QString::number(this->sharedDataP->m_vttAccuracy * 100.0, 'f', 1) + "%";
    clipInfo[QStringLiteral("vtt_accuracy")]            = accuracyStr;

    clipInfo[QStringLiteral("total_events")]            = this->sharedDataP->m_totalEvents;
    clipInfo[QStringLiteral("analysis_date")]           = this->sharedDataP->m_analysisDate;

    obj[QStringLiteral("video_clip_info")] = clipInfo;

    // --- VTT Verification ---
    QJsonObject vttVerif;
    vttVerif[QStringLiteral("confirmed")] = QJsonArray::fromStringList(this->sharedDataP->m_vttConfirmedLog);
    vttVerif[QStringLiteral("rejected")]  = QJsonArray::fromStringList(this->sharedDataP->m_vttRejectedLog);

    obj[QStringLiteral("vtt_verification")] = vttVerif;

    return obj;
}

VideoSegmentInfo VideoSegmentInfo::fromJson(const QJsonObject &json)
{
    // 1. Reconstruct IDs
    UniqueId sId  = UniqueId::fromString(json[QStringLiteral("segment_id")].toString());
    UniqueId fvId = UniqueId::fromString(json[QStringLiteral("fullvideo_id")].toString());

    quint32 sidx = 0;

    QJsonValue val = json[QStringLiteral("segment_index")];
    if (val.isDouble())
    {
        sidx = static_cast<quint32>(val.toDouble());
    }

    // 2. Initialize using the parameterized constructor (locks the IDs)
    VideoSegmentInfo seg(
        json[QStringLiteral("segment_path")].toString(),
        sId,
        fvId,
        json[QStringLiteral("start_time")].toDouble(),
        json[QStringLiteral("end_time")].toDouble(),
        json[QStringLiteral("duration")].toDouble(),
        json[QStringLiteral("state")].toString(),
        sidx
        );

    // 3. Deserialize Audio Track
    if (json.contains(QStringLiteral("audio_track"))) {
        seg.setAudioTrack(Audio::fromJson(json[QStringLiteral("audio_track")].toObject()));
    }

    // 4. Parse Analysis: video_clip_info
    if (json.contains(QStringLiteral("video_clip_info"))) {
        QJsonObject clipInfo = json[QStringLiteral("video_clip_info")].toObject();

        seg.sharedDataP->m_slowdownFactor = clipInfo[QStringLiteral("slowdown_factor")].toInt();
        seg.sharedDataP->m_totalEvents    = clipInfo[QStringLiteral("total_events")].toInt();
        seg.sharedDataP->m_analysisDate   = clipInfo[QStringLiteral("analysis_date")].toString();

        // Convert Priority Events Array to QStringList
        QJsonArray priorityArr = clipInfo[QStringLiteral("priority_events_detected")].toArray();

        for (const QJsonValue &val : qAsConst(priorityArr))
        {
            seg.sharedDataP->m_priorityEvents.append(val.toString());
        }

        // Convert "66.7%" string to 0.667 double
        QString accStr = clipInfo[QStringLiteral("vtt_accuracy")].toString();
        if (accStr.endsWith('%'))
        {
            accStr.chop(1); // Remove '%'
            seg.sharedDataP->m_vttAccuracy = accStr.toDouble() / 100.0;
        }
    }

    // 5. Parse Analysis: vtt_verification
    if (json.contains(QStringLiteral("vtt_verification")))
    {
        QJsonObject vttVerif = json[QStringLiteral("vtt_verification")].toObject();

        // Helper to convert JsonArray to QStringList
        auto jsonArrayToStringList = [](const QJsonArray &arr) -> QStringList {
            QStringList list;
            for (const QJsonValue &v : arr) list << v.toString();
            return list;
        };

        seg.sharedDataP->m_vttConfirmedLog = jsonArrayToStringList(vttVerif[QStringLiteral("confirmed")].toArray());
        seg.sharedDataP->m_vttRejectedLog  = jsonArrayToStringList(vttVerif[QStringLiteral("rejected")].toArray());
    }

    return seg;
}

void VideoSegmentInfo::parseEvtsJsonToTxt(const QString& evtsJsonPath, const QString& outputPath)
{
    QFile inputFile(evtsJsonPath);
    if (!inputFile.open(QIODevice::ReadOnly))
    {
        qCritical() << "Error: Could not open JSON file:" << evtsJsonPath;
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(inputFile.readAll(), &parseError);
    inputFile.close();

    if (parseError.error != QJsonParseError::NoError)
    {
        qCritical() << "JSON Parse Error:" << parseError.errorString();
        return;
    }

    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&outputFile);
    out.setCodec("UTF-8");
    out << "VIDEO SEGMENT ANALYSIS REPORT\n";
    out << QString(60, '=') << "\n\n";

    // Handle both single object or array of segments
    QJsonArray dataArray = doc.isArray() ? doc.array() : QJsonArray{doc.object()};

    for (const QJsonValue &value : qAsConst(dataArray))
    {
        if (!value.isObject()) continue;
        QJsonObject obj = value.toObject();

        // 1. Core Identifiers (Standardized to use toHex/fromString logic)
        UniqueId segId  = UniqueId::fromString(obj.value("segment_id").toString());
        UniqueId fVidId = UniqueId::fromString(obj.value("fullvideo_id").toString());

        quint32 sidx = 0;

        QJsonValue val = obj[QStringLiteral("segment_index")];

        if (val.isDouble())
        {
            sidx = static_cast<quint32>(val.toDouble());
        }

        out << "SEGMENT ID:    " << segId.toHex() << "\n";
        out << "Segment Index: " << sidx << "\n";
        out << "Full Video:    " << fVidId.toHex() << "\n";
        out << "File Path:     " << obj.value("segment_path").toString() << "\n";
        out << "Timing:        " << obj.value("start_time").toDouble() << "s - "
            << obj.value("end_time").toDouble() << "s\n";
        out << "State:         " << obj.value("state").toString() << "\n";

        // 2. Analysis Data (video_clip_info)
        if (obj.contains("video_clip_info")) {
            QJsonObject clip = obj.value("video_clip_info").toObject();
            out << "\n[AI CLIP ANALYSIS]\n";
            out << " - Slowdown Factor: " << clip.value("slowdown_factor").toInt() << "x\n";
            out << " - VTT Accuracy:    " << clip.value("vtt_accuracy").toString() << "\n";
            out << " - Analysis Date:   " << clip.value("analysis_date").toString() << "\n";

            QJsonArray priority = clip.value("priority_events_detected").toArray();
            QStringList pList;

            for(const auto &p : qAsConst(priority)) pList << p.toString();

            out << " - Detected:        " << pList.join(", ") << "\n";
        }

        // 3. VTT Verification Logs
        if (obj.contains("vtt_verification"))
        {
            QJsonObject vtt = obj.value("vtt_verification").toObject();
            out << "\n[VTT VERIFICATION LOG]\n";

            QJsonArray confirmed = vtt.value("confirmed").toArray();
            out << " - Confirmed Events (" << confirmed.size() << "):\n";

            for(const auto &c : qAsConst(confirmed)) out << "     * " << c.toString() << "\n";

            QJsonArray rejected = vtt.value("rejected").toArray();
            if (!rejected.isEmpty())
            {
                out << " - Rejected/False Positives:\n";

                for(const auto &r : qAsConst(rejected)) out << "     * " << r.toString() << "\n";
            }
        }

        // 4. Audio Track Info
        if (obj.contains("audio_track"))
        {
            QJsonObject aud = obj.value("audio_track").toObject();
            out << "\n[AUDIO DATA]\n";
            out << " - Language: " << aud.value("language").toString() << "\n";

            if (!aud.value("transcription").toObject().isEmpty())
            {
                out << " - Transcription: Available\n";
            }
        }

        out << "\n" << QString(60, '-') << "\n\n";
    }

    outputFile.close();
    qDebug() << "Report generated successfully at:" << outputPath;
}

void VideoSegmentInfo::saveSegmentJson(const VideoSegmentInfo &seg, const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qCritical() << "VideoSegmentInfo: Could not open file for writing:" << filename;

        return;
    }

    // toJson() now automatically includes all new VideoSegmentInfoData fields
    QJsonDocument doc(seg.toJson());

    // Using Indented format makes it easier for humans (and Gemini) to read
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

VideoSegmentInfo::VideoSegmentInfoList_t VideoSegmentInfo::loadSegmentsListJson(const QString &filename)
{
    VideoSegmentInfoList_t list;
    QFile file(filename);

    if (!file.exists())
    {
        qWarning() << "VideoSegmentInfo: File does not exist:" << filename;
        return list;
    }

    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "VideoSegmentInfo: Could not open file for reading:" << filename;
        return list;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError)
    {
        qCritical() << "VideoSegmentInfo: JSON Parse Error in" << filename
                    << ":" << parseError.errorString();
        return list;
    }

    // Support both a JSON Array of segments or a single JSON Object segment
    if (doc.isArray())
    {
        QJsonArray array = doc.array();

        for (const QJsonValue &value : qAsConst(array))
        {
            if (value.isObject())
            {
                list.append(VideoSegmentInfo::fromJson(value.toObject()));
            }
        }
    }
    else if (doc.isObject()) {
        list.append(VideoSegmentInfo::fromJson(doc.object()));
    }

    return list;
}

void VideoSegmentInfo::saveSegmentsListJsonAdvanced(VideoSegmentInfoList_t newList, const QString &filename)
{
    // 1. Load existing data into a map for merging
    // UniqueId must have operator< implemented to work as a QMap key
    QMap<UniqueId, VideoSegmentInfo> combinedMap;

    // loadSegmentsListJson now handles the new Gemini JSON structure
    VideoSegmentInfoList_t existing = loadSegmentsListJson(filename);

    for (const auto &seg : qAsConst(existing))
    {
        if (!seg.id().isEmpty())
        {
            combinedMap.insert(seg.id(), seg);
        }
    }

    // 2. Merge new segments (overwriting by ID)
    // This allows model's updated analysis to replace basic segment data
    for (const auto &seg : newList)
    {
        if (!seg.id().isEmpty())
        {
            combinedMap.insert(seg.id(), seg);
        }
    }

    // 3. Extract and Sort by start_time
    QList<VideoSegmentInfo> sortedList = combinedMap.values();

    std::sort(sortedList.begin(), sortedList.end(), [](const VideoSegmentInfo &a, const VideoSegmentInfo &b) {
        // Safe double comparison for sorting
        if (qFuzzyCompare(a.startTime(), b.startTime())) return false;
        return a.startTime() < b.startTime();
    });

    // 4. Create Backup (Standard Data Integrity Practice)
    if (QFile::exists(filename))
    {
        QString backupPath = filename + ".bak";
        QFile::remove(backupPath);

        if (!QFile::rename(filename, backupPath))
        {
            qWarning() << "VideoSegmentInfo: Could not create backup. File might be locked.";
        }
    }

    // 5. Serialize the merged and sorted list
    QJsonArray array;
    for (const auto &seg : qAsConst(sortedList))
    {
        // toJson() now includes video_clip_info, vtt_verification, and audio_track
        array.append(seg.toJson());
    }

    QFile file(filename);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QJsonDocument doc(array);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        qDebug() << "Advanced Save complete:" << filename << "Total Segments:" << sortedList.size();
    }
    else
    {
        qCritical() << "Critical Error: Could not write merged segments to" << filename;
    }
}

void VideoSegmentInfo::saveSegmentsBinary(const VideoSegmentInfoList_t &list, const QString &filename)
{
    QFile file(filename);

    if (file.open(QIODevice::WriteOnly))
    {
        QDataStream out(&file);
        out << list;
    }
}

VideoSegmentInfo::VideoSegmentInfoList_t VideoSegmentInfo::loadSegmentsBinary(const QString &filename)
{
    VideoSegmentInfoList_t list;
    QFile file(filename);

    if (file.open(QIODevice::ReadOnly))
    {
        QDataStream in(&file);
        in >> list;
    }

    return list;
}

// void VideoSegmentInfo::saveSegmentsToCsv(const VideoSegmentInfoList_t &list, const QString &filename)
// {
//     QFile file(filename);
//     if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

//     QTextStream out(&file);
//     out.setCodec("UTF-8");

//     // Header
//     out << "segment_id,fullvideo_id,start_time,end_time,duration,state,segment_path,audio_id,audio_language,transcription_snippet\n";

//     for (const auto &seg : list)
//     {
//         // Sanitize annotation for CSV (remove newlines, quotes)
//         QJsonDocument doc(seg.audioTrack().annotation());
//         QString jsonStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
//         jsonStr.replace("\"", "'"); // Basic sanitization

//         out << seg.id().toString() << ","
//             << seg.fullVideoId().toString() << ","
//             << QString::number(seg.startTime(), 'f', 2) << ","
//             << QString::number(seg.endTime(), 'f', 2) << ","
//             << QString::number(seg.duration(), 'f', 2) << ","
//             << "\"" << seg.state() << "\","
//             << "\"" << seg.path() << "\","
//             // Audio Data
//             << seg.audioTrack().id().toString() << ","
//             << "\"" << seg.audioTrack().language() << "\","
//             << "\"" << jsonStr << "\"\n";
//     }
// }

void VideoSegmentInfo::saveSegmentsToCsv(const VideoSegmentInfoList_t &list, const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "VideoSegmentInfo: Could not open CSV for writing:" << filename;
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    // 1. Updated Header with Gemini Analysis Columns
    out << "segment_id,fullvideo_id,start_time,end_time,duration,state,segment_path,"
        << "slowdown,accuracy,total_events,analysis_date,priority_events,"
        << "vtt_confirmed_log,vtt_rejected_log,"
        << "audio_id,audio_language,transcription_snippet\n";

    for (const auto &seg : list)
    {
        // 1. Get the string directly
        QString jsonStr = seg.audioTrack().transcription();

        // 2. Sanitize for CSV compatibility:
        // CSV fields are wrapped in double quotes "...", so we must
        // change internal double quotes to single quotes or escape them.
        jsonStr.replace(QLatin1Char('"'), QLatin1Char('\''));

        // Also remove newlines which break CSV rows
        jsonStr.replace(QLatin1Char('\n'), QLatin1Char(' '));
        jsonStr.replace(QLatin1Char('\r'), QLatin1Char(' '));

        // 3. Write the row
        out << seg.id().toHex() << ","
            << seg.segmentIndex() << ","
            << seg.fullVideoId().toHex() << ","
            << QString::number(seg.startTime(), 'f', 2) << ","
            << QString::number(seg.endTime(), 'f', 2) << ","
            << QString::number(seg.duration(), 'f', 2) << ","
            << "\"" << seg.state() << "\","
            << "\"" << seg.filePath() << "\","
            << seg.sharedDataP->m_slowdownFactor << ","
            << "\"" << (QString::number(seg.sharedDataP->m_vttAccuracy * 100.0, 'f', 1) + "%") << "\","
            << seg.sharedDataP->m_totalEvents << ","
            << "\"" << seg.sharedDataP->m_analysisDate << "\","
            << "\"" << seg.sharedDataP->m_priorityEvents.join("; ") << "\","
            << "\"" << seg.sharedDataP->m_vttConfirmedLog.join("; ") << "\","
            << "\"" << seg.sharedDataP->m_vttRejectedLog.join("; ") << "\","
            << seg.audioTrack().id().toHex() << ","
            << "\"" << seg.audioTrack().language() << "\","
            << "\"" << jsonStr << "\"\n"; // jsonStr is now safe inside quotes
    }

    file.close();
    qDebug() << "CSV Export complete:" << filename;
}

void VideoSegmentInfo::saveSegmentsToReport(const VideoSegmentInfoList_t &list, const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "VideoSegmentInfo: Could not open report for writing:" << filename;
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << "VIDEO SEGMENTATION & AI ANALYSIS REPORT\n";
    out << "Generated on: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << QString(50, '=') << "\n\n";

    for (const auto &seg : list)
    {
        // 1. Core Segment Information
        out << "[SEGMENT DATA]\n";
        out << "Index:          " << seg.segmentIndex() << "\n";
        out << "ID:             " << seg.id().toHex() << "\n";
        out << "Full Video ID:  " << seg.fullVideoId().toHex() << "\n";
        out << "File Path:      " << seg.filePath() << "\n";
        out << "Interval:       " << QString::number(seg.startTime(), 'f', 2) << "s - "
            << QString::number(seg.endTime(), 'f', 2) << "s "
            << "(Duration: " << QString::number(seg.duration(), 'f', 2) << "s)\n";
        out << "State:          " << seg.state() << "\n";

        // 2. Gemini Analysis (Clip Info)
        out << "\n[AI ANALYSIS SUMMARY]\n";
        out << "Slowdown Factor: " << seg.sharedDataP->m_slowdownFactor << "x\n";
        out << "VTT Accuracy:    " << (QString::number(seg.sharedDataP->m_vttAccuracy * 100.0, 'f', 1) + "%") << "\n";
        out << "Total Events:    " << seg.sharedDataP->m_totalEvents << "\n";
        out << "Analysis Date:   " << seg.sharedDataP->m_analysisDate << "\n";
        out << "Priority Events: " << seg.sharedDataP->m_priorityEvents.join(", ") << "\n";

        // 3. VTT Verification Logs
        if (!seg.sharedDataP->m_vttConfirmedLog.isEmpty() || !seg.sharedDataP->m_vttRejectedLog.isEmpty()) {
            out << "\n[VTT VERIFICATION LOG]\n";

            if (!seg.sharedDataP->m_vttConfirmedLog.isEmpty()) {
                out << "Confirmed:\n";
                for (const QString &log : seg.sharedDataP->m_vttConfirmedLog)
                    out << "  [OK] " << log << "\n";
            }

            if (!seg.sharedDataP->m_vttRejectedLog.isEmpty()) {
                out << "Rejected/False Positives:\n";
                for (const QString &log : seg.sharedDataP->m_vttRejectedLog)
                    out << "  [X]  " << log << "\n";
            }
        }

        // 4. Audio Track Info
        Audio aud = seg.audioTrack();
        out << "\n[AUDIO TRACK]\n";
        if (!aud.id().isEmpty())
        {
            out << "ID:             " << aud.id().toHex() << "\n";
            out << "Language:       " << aud.language() << "\n";

            // Check transcription (using QString::isEmpty() logic)
            if (!aud.transcription().isEmpty()) {
                out << "Transcription:  (Present)\n";
            } else {
                out << "Transcription:  (None)\n";
            }
        } else {
            out << "Status:         No audio track associated.\n";
        }

        out << "\n" << QString(50, '-') << "\n\n";
    }

    file.close();
}

QDataStream &operator<<(QDataStream &out, const VideoSegmentInfo &obj)
{
    out << obj.sharedDataP->m_filePath
        << obj.sharedDataP->m_id
        << obj.sharedDataP->m_segmentIndex
        << obj.sharedDataP->m_fullVideoId
        << obj.sharedDataP->m_startTime
        << obj.sharedDataP->m_endTime
        << obj.sharedDataP->m_duration
        << obj.sharedDataP->m_state;

    // Stream Audio Track
    // Ideally Audio should have its own operator<<.
    // If not, we stream its members here manually:
    Audio audio = obj.sharedDataP->m_audioTrack;
    out << audio.id()
        << audio.eventId()
        << audio.language()
        << audio.annotation() // QJsonObject is streamable in Qt5
        << audio.fileData();     // QByteArray is streamable

    out << obj.sharedDataP->m_slowdownFactor
        << obj.sharedDataP->m_priorityEvents
        << obj.sharedDataP->m_vttAccuracy
        << obj.sharedDataP->m_totalEvents
        << obj.sharedDataP->m_analysisDate
        << obj.sharedDataP->m_vttConfirmedLog
        << obj.sharedDataP->m_vttRejectedLog;

    return out;
}

QDataStream &operator>>(QDataStream &in, VideoSegmentInfo &obj)
{
    // Ensure this object has its own unique instance block before modifying deep fields
    obj.sharedDataP.detach();

    in >> obj.sharedDataP->m_filePath
        >> obj.sharedDataP->m_id
        >> obj.sharedDataP->m_segmentIndex
        >> obj.sharedDataP->m_fullVideoId
        >> obj.sharedDataP->m_startTime
        >> obj.sharedDataP->m_endTime
        >> obj.sharedDataP->m_duration
        >> obj.sharedDataP->m_state;

    // Read Audio Track
    UniqueId aId, aEventId;
    QString aLang;
    QJsonObject aTrans;
    QByteArray aData;

    in >> aId
            >> aEventId
            >> aLang
            >> aTrans
            >> aData;

    Audio audio;
    audio.setId(aId);
    audio.setEventId(aEventId);
    audio.setLanguage(aLang);
    audio.setAnnotation(aTrans);
    audio.setFileData(aData);

    obj.sharedDataP->m_audioTrack = audio;

    in >> obj.sharedDataP->m_slowdownFactor
        >> obj.sharedDataP->m_priorityEvents
        >> obj.sharedDataP->m_vttAccuracy
        >> obj.sharedDataP->m_totalEvents
        >> obj.sharedDataP->m_analysisDate
        >> obj.sharedDataP->m_vttConfirmedLog
        >> obj.sharedDataP->m_vttRejectedLog;

    return in;
}

static int registerVideoSegmentInfoMetatype()
{
    return qRegisterMetaType<VideoSegmentInfo>("VideoSegmentInfo");
}

static int dummyVideoSegmentInfo = registerVideoSegmentInfoMetatype();

static int registerVideoSegmentInfoListMetatype()
{
    return qRegisterMetaType<VideoSegmentInfo::VideoSegmentInfoList_t>("VideoSegmentInfo::VideoSegmentInfoList_t");
}

static int dummyVideoSegmentInfoList = registerVideoSegmentInfoListMetatype();
