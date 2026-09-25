#ifndef EVENTDATABASEMANAGER_H
#define EVENTDATABASEMANAGER_H

#include <QObject> // Inherit from QObject
#include <QDebug>
#include <QCryptographicHash>
#include <QMap>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSharedPointer>

#include "UniqueId.h"
using namespace utils;

// Include your specific domain headers
#include "inc/EventModel.h"
#include "inc/FullVideo.h"
#include "inc/Actor.h"
#include "inc/VideoSegmentInfo.h"

class EventDatabaseManager : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "Event Database Manager")

    public:

        static EventDatabaseManager& instance();

        /**
         * @brief connectToDatabase
         * @param path
         * @return
         */
        bool connectToDatabase(const QString &path = "multimedia_events.db");

        /**
         * @brief closeDatabase
         */
        void closeDatabase();

        // ==========================================
        // Standalone Audio Operations (NEW)
        // ==========================================

        /**
         * @brief saveAudioSegment
         * @param aud
         * @param linkedVideoSegmentId
         * @return
         */
        bool saveAudioSegment(const Audio &aud, const UniqueId &linkedVideoSegmentId);

        /**
         * @brief getAudioSegment
         * @param audioId
         * @return
         */
        Audio getAudioSegment(const UniqueId &audioId) const;

        /**
         * @brief getAudioSegmentsForFullAudio
         * @param fullAudioId
         * @return
         */
        QList<Audio> getAudioSegmentsForFullAudio(const UniqueId &fullAudioId) const;


        // ==========================================
        // 1. Full Video Management (BLOB Storage)
        // ==========================================

        /**
         * @brief addFullVideo
         * @param video
         * @return
         */
        bool addFullVideo(const FullVideo &video);

        /**
         * @brief getFullVideo
         * @param videoId
         * @return
         */
        FullVideo getFullVideo(const UniqueId &videoId) const;

        /**
         * @brief getAllFullVideos
         * @return
         */
        QList<FullVideo> getAllFullVideos() const;

        // ==========================================
        // 2. Video Segment & Audio Management
        // ==========================================

        /**
         * @brief addVideoSegment
         * @param seg
         * @return
         */
        bool addVideoSegment(const VideoSegmentInfo &seg);

        /**
         * @brief getSegment
         * @param segmentId
         * @return
         */
        VideoSegmentInfo getSegment(const UniqueId &segmentId) const;

        /**
         * @brief getSegmentsForVideo
         * @param fullVideoId
         * @return
         */
        QList<VideoSegmentInfo> getSegmentsForVideo(const UniqueId &fullVideoId) const;

        // ==========================================
        // 3. Event Management (Recursive)
        // ==========================================

        /**
         * @brief saveEvent
         * @param event
         * @return
         */
        bool saveEvent(const QSharedPointer<Event> &event);

        /**
         * @brief deleteEvent
         * @param eventId
         * @return
         */
        bool deleteEvent(const UniqueId &eventId);

        /**
         * @brief getAllRootEvents
         * @return
         */
        QList<QSharedPointer<Event>> getAllRootEvents() const;

        /**
         * @brief findEventsByHash: Deep search for events based on cryptographic hash components
         * Public so the UI or search controllers can use it.
         * @param hash
         * @param algo
         * @return
         */
        QList<QSharedPointer<Event>> findEventsByHash(const QByteArray &hash,
                                                      QCryptographicHash::Algorithm algo) const;

        /**
         * @brief findEventsByCRC: Find events using only the 16-bit checksum
         * @param crc
         * @return
         */
        QList<QSharedPointer<Event>> findEventsByCRC(quint16 crc) const;

        /**
         * @brief getEventById
         * @param hexId
         * @return
         */
        QSharedPointer<Event> getEventById(const QString &hexId) const;

        /**
         * @brief searchEvents
         * @param text
         * @return
         */
        QList<QSharedPointer<Event>> searchEvents(const QString &text) const;

        /**
         * @brief getEventsForFullVideo
         * @param fullVideoId
         * @return
         */
        QList<QSharedPointer<Event>> getEventsForFullVideo(const UniqueId &fullVideoId) const;

        /**
         * @brief getTimelineForVideo
         * @param fullVideoId
         * @return
         */
        QList<QSharedPointer<Event>> getTimelineForVideo(const UniqueId &fullVideoId) const;

        // ==========================================
        // 4. Import data from Json
        // ==========================================

        bool batchImportFromJson(const QJsonArray &eventArray);

        /**
         * @brief exportMatchPackage
         * @param rootEventHexId
         * @param destinationDir
         * @return
         */
        bool exportMatchPackage(const QString &rootEventHexId, const QString &destinationDir);

        /**
         * @brief importMatchPackage
         * @param packageDir
         * @return
         */
        bool importMatchPackage(const QString &packageDir);

    signals:

        /**
         * @brief dbConnected
         */
        void dbConnected();

        /**
         * @brief errorOccurred
         * @param errorMsg
         */
        void errorOccurred(const QString &errorMsg);

        /**
         * @brief eventSaved
         * @param eventId
         */
        void eventSaved(const QString &eventId);

        /**
         * @brief fullVideoSaved
         * @param videoId
         */
        void fullVideoSaved(const UniqueId &videoId);

    private:

        explicit EventDatabaseManager(QObject *parent = nullptr);

        ~EventDatabaseManager();

        // Prevent copy
        EventDatabaseManager(const EventDatabaseManager&) = delete;
        EventDatabaseManager& operator=(const EventDatabaseManager&) = delete;

        /**
         * @brief createTables
         */
        void createTables();

        // Helpers

        /**
         * @brief saveEventRecursive
         * @param event
         * @param parentId
         * @return
         */
        bool saveEventRecursive(const QSharedPointer<Event> &event, const UniqueId &parentId);

        /**
         * @brief saveActors
         * @param eventId
         * @param actors
         * @return
         */
        bool saveActors(const UniqueId &eventId, const QList<Actor> &actors);

        /**
         * @brief getActor
         * @param searchId
         * @return
         */
        Actor getActor(const UniqueId &searchId);

        /**
         * @brief getActorsForEvent
         * @param eventId
         * @return
         */
        QList<Actor> getActorsForEvent(const UniqueId &eventId) const;

    private:

        mutable QMutex m_mutex; // Mutable allows locking in const functions

};

#endif // EVENTDATABASEMANAGER_H
