#ifndef VIDEOSEGMENTINFO_H
#define VIDEOSEGMENTINFO_H

#include <QString>
#include <QJsonObject>
#include <QList>
#include <QMetaType>

#include <QSharedDataPointer>

#include "UniqueId.h"
using namespace utils;

#include "inc/Audio.h"

class VideoSegmentInfoData;

class VideoSegmentInfo
{
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        Q_PROPERTY(QString filePath READ filePath WRITE setFilePath)
        Q_PROPERTY(UniqueId id READ id WRITE setId)
        Q_PROPERTY(UniqueId fullVideoId READ fullVideoId WRITE setFullVideoId)
        Q_PROPERTY(QString state READ state WRITE setState)
        Q_PROPERTY(Audio audioTrack READ audioTrack WRITE setAudioTrack)

        /**
         * @brief classVersion
         */
        static const quint16 classVersion = quint16(101);

    public:

        /**
         * @brief VideoSegmentInfo
         * @param parent
         */
        explicit VideoSegmentInfo();

        /**
         * @brief VideoSegmentInfo
         * @param path
         * @param id
         * @param fullVideoId
         * @param start
         * @param end
         * @param duration
         * @param state
         */
        VideoSegmentInfo(const QString & path,
                         const UniqueId & id,
                         const UniqueId & fullVideoId,
                         const double start,
                         const double end,
                         const double duration,
                         const QString & state,
                         const quint32 index);

        /**
         * @brief ~VideoSegmentInfo
         */
        virtual ~VideoSegmentInfo();

        /**
         * @brief VideoSegmentInfo
         * @param rhs
         */
        VideoSegmentInfo(const VideoSegmentInfo &rhs);

        /**
         * @brief operator =
         * @param right
         * @return
         */
        VideoSegmentInfo &operator=(const VideoSegmentInfo &right);

        /**
         * @brief operator ==
         * @param right
         * @return
         */
        bool operator ==(const VideoSegmentInfo &right) const;

        /**
         * @brief operator !=
         * @param right
         * @return
         */
        bool operator !=(const VideoSegmentInfo &right) const;

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const VideoSegmentInfo *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const VideoSegmentInfo *ptr) const;

        /**
         * Typedef inside the class for clean namespacing
         */
        using VideoSegmentInfoList_t = QList<VideoSegmentInfo>;

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
         * @brief segmentIndex
         * @return
         */
        qint32 segmentIndex() const;

        /**
         * @brief setSegmentIndex
         * @param newIndex
         */
        void setSegmentIndex(qint32 newIndex);

        /**
         * @brief fullVideoId
         * @return
         */
        UniqueId fullVideoId() const;

        /**
         * @brief setFullVideoId
         * @param newVId
         */
        void setFullVideoId(const UniqueId &newVId);

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
         * @brief audioTrack
         * @return
         */
        Audio audioTrack() const;

        /**
         * @brief setAudioTrack
         * @param newAudio
         */
        void setAudioTrack(const Audio &newAudio);

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
         * @brief slowdownFactor
         * @return
         */
        int slowdownFactor() const;

        /**
         * @brief setSlowdownFactor
         * @param newSlowdownFactor
         */
        void setSlowdownFactor(qreal newSlowdownFactor);

        /**
         * @brief priorityEvents
         * @return
         */
        QStringList priorityEvents() const;

        /**
         * @brief setPriorityEvents
         * @param newPriorityEvents
         */
        void setPriorityEvents(const QStringList &newPriorityEvents);

        /**
         * @brief vttAccuracy
         * @return
         */
        double vttAccuracy() const;

        /**
         * @brief setVttAccuracy
         * @param newVttAccuracy
         */
        void setVttAccuracy(double newVttAccuracy);

        /**
         * @brief totalEvents
         * @return
         */
        int totalEvents() const;

        /**
         * @brief setTotalEvents
         * @param newTotalEvents
         */
        void setTotalEvents(qint32 newTotalEvents);

        /**
         * @brief analysisDate
         * @return
         */
        QString analysisDate() const;

        /**
         * @brief setAnalysisDate
         * @param newAnalysisDate
         */
        void setAnalysisDate(const QString &newAnalysisDate);

        /**
         * @brief vttConfirmedLog
         * @return
         */
        QStringList vttConfirmedLog() const;

        /**
         * @brief setVttConfirmedLog
         * @param newVttConfirmedLog
         */
        void setVttConfirmedLog(const QStringList &newVttConfirmedLog);

        /**
         * @brief vttRejectedLog
         * @return
         */
        QStringList vttRejectedLog() const;

        /**
         * @brief setVttRejectedLog
         * @param newVttRejectedLog
         */
        void setVttRejectedLog(const QStringList &newVttRejectedLog);

        /**
         * @brief event
         * @return
         */
        QString event() const;

        /**
         * @brief setEvent
         * @param newEvent
         */
        void setEvent(const QString & newEvent);

        /**
         * @brief toJson
         * @return
         */
        QJsonObject toJson() const;

        /**
         * @brief fromJson
         * @param json
         */
        static VideoSegmentInfo fromJson(const QJsonObject &json);

        /**
         * @brief parseEvtsJsonToTxt
         * @param evtsJsonPath
         * @param outputPath
         */
        static void parseEvtsJsonToTxt(const QString & evtsJsonPath,
                                       const QString & outputPath);

        // save_segment_json

        /**
         * @brief saveSegmentJson
         * @param seg
         * @param filename
         */
        static void saveSegmentJson(const VideoSegmentInfo &seg, const QString &filename);

        // load_segments_list_json_advanced

        /**
         * @brief loadSegmentsListJson
         * @param filename
         * @return
         */
        static VideoSegmentInfoList_t loadSegmentsListJson(const QString &filename);

        // save_segments_list_json (The Merge & Sort Logic)

        /**
         * @brief saveSegmentsListJsonAdvanced
         * @param newList
         * @param filename
         */
        static void saveSegmentsListJsonAdvanced(VideoSegmentInfoList_t newList, const QString &filename);

        // save_segments_pickle

        /**
         * @brief saveSegmentsBinary
         * @param list
         * @param filename
         */
        static void saveSegmentsBinary(const VideoSegmentInfoList_t &list, const QString &filename);

        // load_segments_pickle

        /**
         * @brief loadSegmentsBinary
         * @param filename
         * @return
         */
        static VideoSegmentInfoList_t loadSegmentsBinary(const QString &filename);

        // save_segments_to_csv

        /**
         * @brief saveSegmentsToCsv
         * @param list
         * @param filename
         */
        static void saveSegmentsToCsv(const VideoSegmentInfoList_t &list, const QString &filename);

        // save_segments_to_report

        /**
         * @brief saveSegmentsToReport
         * @param list
         * @param filename
         */
        static void saveSegmentsToReport(const VideoSegmentInfoList_t &list, const QString &filename);

        /**
         * @brief operator <<
         * @param out
         * @param obj
         * @return
         */
        friend QDataStream &operator<<(QDataStream &out, const VideoSegmentInfo &obj);

        /**
         * @brief operator >>
         * @param in
         * @param obj
         * @return
         */
        friend QDataStream &operator>>(QDataStream &in, VideoSegmentInfo &obj);

    signals:

        // TODO

    private:

        QSharedDataPointer<VideoSegmentInfoData> sharedDataP;

};

inline uint qHash(const VideoSegmentInfo &key, uint seed = 0)
{
    // Use Qt's built-in qHash for the internal members
    // Combine them using XOR or Qt's internal hash combining logic
    return qHash(key.id(), seed) ^
           qHash(key.filePath(), seed) ^
           qHash(key.startTime(), seed);
}

Q_DECLARE_METATYPE(VideoSegmentInfo)

Q_DECLARE_METATYPE(VideoSegmentInfo::VideoSegmentInfoList_t)

#endif // VIDEOSEGMENTINFO_H
