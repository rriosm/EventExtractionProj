
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QVariant>

#include "inc/EventDatabaseManager.h"

EventDatabaseManager& EventDatabaseManager::instance()
{
    static EventDatabaseManager instance;
    return instance;
}

EventDatabaseManager::EventDatabaseManager(QObject *parent) : QObject(parent)
{
    // TUDO
}

EventDatabaseManager::~EventDatabaseManager()
{
    this->closeDatabase();
}

bool EventDatabaseManager::connectToDatabase(const QString &path)
{
    QMutexLocker locker(&this->m_mutex);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);

    if (!db.open())
    {
        QString err = "Connection failed: " + db.lastError().text();
        qCritical() << "EventDatabaseManager:" << err;
        emit errorOccurred(err);
        return false;
    }

    // Enable Foreign Keys for SQLite (Essential for CASCADE delete)
    QSqlQuery query("PRAGMA foreign_keys = ON;");

    if (!query.exec())
    {
        qWarning() << "EventDatabaseManager: Failed to enable foreign keys.";
    }

    createTables();

    emit dbConnected();

    return true;
}

void EventDatabaseManager::closeDatabase()
{
    QMutexLocker locker(&m_mutex);
    QString connectionName = QSqlDatabase::database().connectionName();
    QSqlDatabase::database().close();
    QSqlDatabase::removeDatabase(connectionName);
}

void EventDatabaseManager::createTables()
{
    QSqlQuery query;
    bool ok = true;

    // 1. Full Videos
    ok &= query.exec("CREATE TABLE IF NOT EXISTS full_videos ("
                     "video_id TEXT PRIMARY KEY, "
                     "name TEXT, "
                     "match_date TEXT, "
                     "file_data BLOB)");

    // 2. Video Segments
    ok &= query.exec("CREATE TABLE IF NOT EXISTS video_segments ("
                     "segment_id TEXT PRIMARY KEY, "
                     "full_video_id TEXT, "
                     "start_time REAL, "
                     "end_time REAL, "
                     "duration REAL, "
                     "state TEXT, "
                     "segment_path TEXT, "
                     "slowdown_factor INTEGER, "
                     "vtt_accuracy REAL, "
                     "total_events INTEGER, "
                     "analysis_date TEXT, "
                     "priority_events TEXT, "
                     "vtt_confirmed_log TEXT, "
                     "vtt_rejected_log TEXT, "
                     "FOREIGN KEY(full_video_id) REFERENCES full_videos(video_id) ON DELETE CASCADE)");

    // 3. Audios (Refactored to mirror the new Audio class schema)
    // segment_id is kept to maintain the link to Video Segments, but is allowed to be NULL for standalone audio.
    ok &= query.exec("CREATE TABLE IF NOT EXISTS audios ("
                     "audio_id TEXT PRIMARY KEY, "
                     "segment_id TEXT, "
                     "full_audio_id TEXT, "
                     "event_id TEXT, "
                     "start_time REAL, "
                     "end_time REAL, "
                     "duration REAL, "
                     "state TEXT, "
                     "file_path TEXT, "
                     "transcription TEXT, "
                     "annotation TEXT, "
                     "language TEXT, "
                     "file_data BLOB, "
                     "FOREIGN KEY(segment_id) REFERENCES video_segments(segment_id) ON DELETE CASCADE)");

    // 4. Events
    ok &= query.exec("CREATE TABLE IF NOT EXISTS events ("
                     "event_id TEXT PRIMARY KEY, "
                     "parent_id TEXT, "
                     "linked_media_id TEXT, "
                     "type TEXT, "
                     "name TEXT, "
                     "description TEXT, "
                     "start_time TEXT, "
                     "end_time TEXT, "
                     "duration REAL, "
                     "concept TEXT, "
                     "media_path TEXT, "
                     "loc_name TEXT, "
                     "loc_city TEXT, "
                     "loc_street TEXT, "
                     "loc_road TEXT, "
                     "loc_number TEXT, "
                     "loc_country TEXT, "
                     "priority_level INTEGER, "
                     "category TEXT, "
                     "expert_analysis TEXT, "
                     "verdict TEXT, "
                     "confidence TEXT, "
                     "vtt_status TEXT, "
                     "FOREIGN KEY(parent_id) REFERENCES events(event_id) ON DELETE CASCADE,"
                     "FOREIGN KEY(linked_media_id) REFERENCES video_segments(segment_id) ON DELETE SET NULL)");

    // 5. Actors
    ok &= query.exec("CREATE TABLE IF NOT EXISTS actors ("
                     "actor_id TEXT PRIMARY KEY, "
                     "name TEXT, "
                     "role TEXT,"
                     "number INTEGER)");

    // 6. Event-Actors Junction
    ok &= query.exec("CREATE TABLE IF NOT EXISTS event_actors ("
                     "event_id TEXT, "
                     "actor_id TEXT, "
                     "PRIMARY KEY (event_id, actor_id), "
                     "FOREIGN KEY(event_id) REFERENCES events(event_id) ON DELETE CASCADE, "
                     "FOREIGN KEY(actor_id) REFERENCES actors(actor_id))");

    // Views
    ok &= query.exec("CREATE VIEW IF NOT EXISTS view_video_timeline AS "
                     "SELECT vs.full_video_id, e.* "
                     "FROM events e "
                     "JOIN video_segments vs ON e.linked_media_id = vs.segment_id "
                     "ORDER BY e.start_time ASC");

    // --- PERFORMANCE INDEXES ---
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_segments_full_video ON video_segments(full_video_id);");
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_audios_segment ON audios(segment_id);");
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_audios_full_audio ON audios(full_audio_id);"); // New Index
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_events_parent ON events(parent_id);");
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_junction_actor ON event_actors(actor_id);");
    ok &= query.exec("CREATE INDEX IF NOT EXISTS idx_events_linked_media ON events(linked_media_id);");

    // --- FULL TEXT SEARCH (FTS5) ---
    ok &= query.exec("CREATE VIRTUAL TABLE IF NOT EXISTS events_search USING fts5("
                     "event_id UNINDEXED, name, description, concept);");

    ok &= query.exec("CREATE TRIGGER IF NOT EXISTS events_ai AFTER INSERT ON events BEGIN "
                     "INSERT INTO events_search(event_id, name, description, concept) "
                     "VALUES (new.event_id, new.name, new.description, new.concept); END;");

    ok &= query.exec("CREATE TRIGGER IF NOT EXISTS events_ad AFTER DELETE ON events BEGIN "
                     "DELETE FROM events_search WHERE event_id = old.event_id; END;");

    ok &= query.exec("CREATE TRIGGER IF NOT EXISTS events_au AFTER UPDATE ON events BEGIN "
                     "UPDATE events_search SET name = new.name, description = new.description, concept = new.concept "
                     "WHERE event_id = old.event_id; END;");

    if (!ok) {
        QString err = "Table creation failed: " + query.lastError().text();
        qCritical() << "EventDatabaseManager:" << err;
        emit errorOccurred(err);
    }
}

// ==========================================
// Standalone Audio Operations
// ==========================================

bool EventDatabaseManager::saveAudioSegment(const Audio &aud, const UniqueId &linkedVideoSegmentId)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery qAud;

    qAud.prepare("INSERT OR REPLACE INTO audios "
                 "(audio_id, segment_id, full_audio_id, event_id, start_time, end_time, "
                 "duration, state, file_path, transcription, annotation, language, file_data) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    qAud.addBindValue(aud.id().toHex());
    qAud.addBindValue(linkedVideoSegmentId.isEmpty() ? QVariant(QVariant::String) : linkedVideoSegmentId.toHex());
    qAud.addBindValue(aud.fullAudioId().isEmpty() ? QVariant(QVariant::String) : aud.fullAudioId().toHex());
    qAud.addBindValue(aud.eventId().isEmpty() ? QVariant(QVariant::String) : aud.eventId().toHex());
    qAud.addBindValue(aud.startTime());
    qAud.addBindValue(aud.endTime());
    qAud.addBindValue(aud.duration());
    qAud.addBindValue(aud.state());
    qAud.addBindValue(aud.filePath());
    qAud.addBindValue(aud.transcription());

    QJsonDocument doc;
    doc.setObject(aud.annotation());
    qAud.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    qAud.addBindValue(aud.language());
    qAud.addBindValue(aud.fileData());

    if (!qAud.exec()) {
        emit errorOccurred("Save Audio Error: " + qAud.lastError().text());
        return false;
    }
    return true;
}

Audio EventDatabaseManager::getAudioSegment(const UniqueId &audioId) const
{
    QSqlQuery qAud;
    qAud.prepare("SELECT * FROM audios WHERE audio_id = ?");
    qAud.addBindValue(audioId.toHex());

    if (qAud.exec() && qAud.next())
    {
        // 1. Extract optional IDs
        QString eIdStr = qAud.value("event_id").toString();
        UniqueId eid = eIdStr.isEmpty() ? UniqueId() : UniqueId::fromString(eIdStr);

        QString faIdStr = qAud.value("full_audio_id").toString();
        UniqueId faId = faIdStr.isEmpty() ? UniqueId() : UniqueId::fromString(faIdStr);

        // 2. Parse JSON Annotation
        QByteArray annotationBytes = qAud.value("annotation").toString().toUtf8();
        QJsonObject annotationObj = QJsonDocument::fromJson(annotationBytes).object();

        // 3. Construct using the exact 8-parameter signature
        Audio audio(
            UniqueId::fromString(qAud.value("audio_id").toString()), // newId
            faId,                                                    // fullAudioId
            eid,                                                     // newEvtId
            qAud.value("language").toString(),                       // newLang
            annotationObj,                                           // newAnnotation
            qAud.value("transcription").toString(),                  // newTranscription
            qAud.value("file_data").toByteArray(),                   // newData
            qAud.value("file_path").toString()                       // path
            );

        // 4. Set remaining properties
        audio.setTiming(qAud.value("start_time").toDouble(), qAud.value("end_time").toDouble());
        audio.setState(qAud.value("state").toString());

        return audio;
    }

    return Audio();
}

QList<Audio> EventDatabaseManager::getAudioSegmentsForFullAudio(const UniqueId &fullAudioId) const
{
    QList<Audio> list;
    QSqlQuery query;
    query.prepare("SELECT audio_id FROM audios WHERE full_audio_id = ? ORDER BY start_time ASC");
    query.addBindValue(fullAudioId.toHex());

    if (query.exec()) {
        while(query.next()) {
            list.append(this->getAudioSegment(UniqueId::fromString(query.value(0).toString())));
        }
    }
    return list;
}

// ==========================================
// Full Video Operations
// ==========================================

bool EventDatabaseManager::addFullVideo(const FullVideo &video)
{
    QMutexLocker locker(&m_mutex);

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO full_videos (video_id, name, match_date, file_data) "
                  "VALUES (?, ?, ?, ?)");

    query.addBindValue(video.id().toHex());
    query.addBindValue(video.name());
    query.addBindValue(video.matchDate().toString(Qt::ISODate));
    query.addBindValue(video.fileData()); // Automatic BLOB binding

    if (query.exec())
    {
        emit fullVideoSaved(video.id());
        return true;
    } else {
        emit errorOccurred("EventDatabaseManager - Add FullVideo Error: " + query.lastError().text());
        return false;
    }
}

FullVideo EventDatabaseManager::getFullVideo(const UniqueId &videoId) const
{
    QSqlQuery query;
    query.prepare("SELECT * FROM full_videos WHERE video_id = ?");
    query.addBindValue(videoId.toHex());

    if (query.exec() && query.next())
    {
        return FullVideo(
            UniqueId::fromString(query.value("video_id").toString()),
            query.value("name").toString(),
            QDateTime::fromString(query.value("match_date").toString(), Qt::ISODate),
            query.value("file_data").toByteArray()
        );
    }

    return FullVideo();
}

QList<FullVideo> EventDatabaseManager::getAllFullVideos() const
{
    QList<FullVideo> list;
    QSqlQuery query("SELECT * FROM full_videos");

    while(query.next())
    {
        list.append(FullVideo(
            UniqueId::fromString(query.value("video_id").toString()),
            query.value("name").toString(),
            QDateTime::fromString(query.value("match_date").toString(), Qt::ISODate),
            query.value("file_data").toByteArray()
        ));
    }
    return list;
}

// ==========================================
// Video Segment & Audio Operations
// ==========================================

bool EventDatabaseManager::addVideoSegment(const VideoSegmentInfo &seg)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase db = QSqlDatabase::database();
    db.transaction();

    // 1. Save Segment
    QSqlQuery qSeg;
    qSeg.prepare("INSERT OR REPLACE INTO video_segments "
                 "(segment_id, full_video_id, start_time, end_time, duration, state, segment_path, "
                 "slowdown_factor, vtt_accuracy, total_events, analysis_date, "
                 "priority_events, vtt_confirmed_log, vtt_rejected_log) "
                 "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    qSeg.addBindValue(seg.id().toHex());
    qSeg.addBindValue(seg.fullVideoId().toHex());
    qSeg.addBindValue(seg.startTime());
    qSeg.addBindValue(seg.endTime());
    qSeg.addBindValue(seg.duration());
    qSeg.addBindValue(seg.state());
    qSeg.addBindValue(seg.filePath());
    qSeg.addBindValue(seg.slowdownFactor());
    qSeg.addBindValue(seg.vttAccuracy());
    qSeg.addBindValue(seg.totalEvents());
    qSeg.addBindValue(seg.analysisDate());
    qSeg.addBindValue(seg.priorityEvents().join(";"));
    qSeg.addBindValue(seg.vttConfirmedLog().join(";"));
    qSeg.addBindValue(seg.vttRejectedLog().join(";"));

    if (!qSeg.exec()) {
        db.rollback();
        emit errorOccurred("Save Segment Error: " + qSeg.lastError().text());
        return false;
    }

    // 2. Save Audio (Uses the expanded audios table)
    Audio aud = seg.audioTrack();

    if (!aud.id().isEmpty())
    {
        // Unlocking temporarily so we can reuse the standalone save function cleanly within the transaction.
        // Using the standalone query logic directly prevents recursive deadlocks
        QSqlQuery qAud;
        qAud.prepare("INSERT OR REPLACE INTO audios "
                     "(audio_id, segment_id, full_audio_id, event_id, start_time, end_time, "
                     "duration, state, file_path, transcription, annotation, language, file_data) "
                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

        qAud.addBindValue(aud.id().toHex());
        qAud.addBindValue(seg.id().toHex()); // Link the audio back to the segment!

        qAud.addBindValue(aud.fullAudioId().isEmpty() ? QVariant(QVariant::String) : aud.fullAudioId().toHex());
        qAud.addBindValue(aud.eventId().isEmpty() ? QVariant(QVariant::String) : aud.eventId().toHex());
        qAud.addBindValue(aud.startTime());
        qAud.addBindValue(aud.endTime());
        qAud.addBindValue(aud.duration());
        qAud.addBindValue(aud.state());
        qAud.addBindValue(aud.filePath());
        qAud.addBindValue(aud.transcription());

        QJsonDocument doc;
        doc.setObject(aud.annotation());
        qAud.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

        qAud.addBindValue(aud.language());
        qAud.addBindValue(aud.fileData());

        if (!qAud.exec()) {
            db.rollback();
            emit errorOccurred("Save Audio Error: " + qAud.lastError().text());
            return false;
        }
    }

    return db.commit();
}

VideoSegmentInfo EventDatabaseManager::getSegment(const UniqueId &segmentId) const
{
    QSqlQuery query;
    query.prepare("SELECT * FROM video_segments WHERE segment_id = ?");
    query.addBindValue(segmentId.toHex());

    if (query.exec() && query.next())
    {
        VideoSegmentInfo info(
            query.value("segment_path").toString(),
            UniqueId::fromString(query.value("segment_id").toString()),
            UniqueId::fromString(query.value("full_video_id").toString()),
            query.value("start_time").toDouble(),
            query.value("end_time").toDouble(),
            query.value("duration").toDouble(),
            query.value("state").toString(),
            query.value("index").toLongLong()
            );

        info.setSlowdownFactor(query.value("slowdown_factor").toInt());
        info.setVttAccuracy(query.value("vtt_accuracy").toDouble());
        info.setTotalEvents(query.value("total_events").toInt());
        info.setAnalysisDate(query.value("analysis_date").toString());

        info.setPriorityEvents(query.value("priority_events").toString().split(QLatin1Char(';'), Qt::SkipEmptyParts));
        info.setVttConfirmedLog(query.value("vtt_confirmed_log").toString().split(QLatin1Char(';'), Qt::SkipEmptyParts));
        info.setVttRejectedLog(query.value("vtt_rejected_log").toString().split(QLatin1Char(';'), Qt::SkipEmptyParts));

        // 3. Load Audio
        QSqlQuery qAud;
        qAud.prepare("SELECT * FROM audios WHERE segment_id = ?");
        qAud.addBindValue(segmentId.toHex());

        if (qAud.exec() && qAud.next())
        {
            QString eIdStr = qAud.value("event_id").toString();
            UniqueId eid = eIdStr.isEmpty() ? UniqueId() : UniqueId::fromString(eIdStr);

            QString faIdStr = qAud.value("full_audio_id").toString();
            UniqueId faId = faIdStr.isEmpty() ? UniqueId() : UniqueId::fromString(faIdStr);

            QByteArray annotationBytes = qAud.value("annotation").toString().toUtf8();
            QJsonObject annotationObj = QJsonDocument::fromJson(annotationBytes).object();

            // Use the exact 8-parameter signature here as well
            Audio audio(
                UniqueId::fromString(qAud.value("audio_id").toString()), // newId
                faId,                                                    // fullAudioId
                eid,                                                     // newEvtId
                qAud.value("language").toString(),                       // newLang
                annotationObj,                                           // newAnnotation
                qAud.value("transcription").toString(),                  // newTranscription
                qAud.value("file_data").toByteArray(),                   // newData
                qAud.value("file_path").toString()                       // path
                );

            audio.setTiming(qAud.value("start_time").toDouble(), qAud.value("end_time").toDouble());
            audio.setState(qAud.value("state").toString());

            info.setAudioTrack(audio);
        }

        return info;
    }

    return VideoSegmentInfo();
}

QList<VideoSegmentInfo> EventDatabaseManager::getSegmentsForVideo(const UniqueId &fullVideoId) const
{
    QList<VideoSegmentInfo> list;
    QSqlQuery query;
    query.prepare("SELECT segment_id FROM video_segments WHERE full_video_id = ? ORDER BY start_time ASC");
    query.addBindValue(fullVideoId.toString());

    if (query.exec())
    {
        while(query.next())
        {
            // Reuse single fetch to ensure consistency
            list.append(this->getSegment(UniqueId::fromString(query.value(0).toString())));
        }
    }

    return list;
}

// ==========================================
// Event Management (Recursive)
// ==========================================

bool EventDatabaseManager::saveEvent(const QSharedPointer<Event> &event)
{
    QMutexLocker locker(&m_mutex);
    QSqlDatabase::database().transaction();

    // Start recursion with no parent (Root)
    if (this->saveEventRecursive(event, UniqueId()))
    {
        if (QSqlDatabase::database().commit())
        {
            emit eventSaved(event->id().toHex());
            return true;
        }
    }

    QString err = QSqlDatabase::database().lastError().text();
    QSqlDatabase::database().rollback();
    emit errorOccurred("Save Event Tree Failed: " + err);

    return false;
}

bool EventDatabaseManager::saveEventRecursive(const QSharedPointer<Event> &event, const UniqueId &parentId)
{
    if (!event) return false;

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO events "
                  "(event_id, parent_id, linked_media_id, type, name, description, start_time, end_time, duration, "
                  "concept, media_path, loc_name, loc_city, loc_street, loc_road, loc_number, loc_country, "
                  "priority_level, category, expert_analysis, verdict, confidence, vtt_status) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(event->id().toHex());
    query.addBindValue(parentId.isEmpty() ? QVariant(QVariant::String) : parentId.toHex());
    query.addBindValue(event->linkedMediaId().isEmpty() ? QVariant(QVariant::String) : event->linkedMediaId().toHex());
    query.addBindValue(event->isAtomic() ? "Atomic" : "Compound");
    query.addBindValue(event->name());
    query.addBindValue(event->description());
    query.addBindValue(event->startTime().toString(Qt::ISODate));
    query.addBindValue(event->endTime().toString(Qt::ISODate));
    query.addBindValue(event->duration());
    query.addBindValue(event->concept());
    query.addBindValue(event->mediaPath());

    // Location Mapping
    Location loc = event->location();
    query.addBindValue(loc.name());
    query.addBindValue(loc.city());
    query.addBindValue(loc.street());
    query.addBindValue(loc.road());   // Preserved
    query.addBindValue(loc.number()); // Preserved
    query.addBindValue(loc.country());

    // Gemini Analytical Mapping
    query.addBindValue(event->priorityLevel());
    query.addBindValue(event->category());
    query.addBindValue(event->expertAnalysis());
    query.addBindValue(event->verdict());
    query.addBindValue(event->confidence());
    query.addBindValue(event->vttStatus());

    if (!query.exec()) return false;

    // Save Actors (Values converts QHash to QList)
    if (!this->saveActors(event->id(), event->actors().values())) return false;

    if (!event->isAtomic())
    {
        auto compound = event.staticCast<CompoundEvent>();
        for (const auto &sub : compound->subEvents())
        {
            if (!this->saveEventRecursive(sub, event->id())) return false;
        }
    }

    return true;
}

bool EventDatabaseManager::saveActors(const UniqueId &eventId, const QList<Actor> &actors)
{
    QSqlQuery qActor;
    qActor.prepare("INSERT OR REPLACE INTO actors (actor_id, name, role, number) VALUES (?, ?, ?, ?)");

    QSqlQuery qLink;
    qLink.prepare("INSERT INTO event_actors (event_id, actor_id) VALUES (?, ?)");

    // Clear old links to handle updates cleanly
    QSqlQuery delLink;
    delLink.prepare("DELETE FROM event_actors WHERE event_id = ?");

    delLink.addBindValue(eventId.toHex());

    if (!delLink.exec())
    {
        qWarning() << "Failed to clear actor links:" << delLink.lastError().text();

        return false;
    }

    for (const Actor &a : actors)
    {
        QString actorHexId = a.id().toHex();

        // 1. Upsert Actor
        qActor.bindValue(0, actorHexId);
        qActor.bindValue(1, a.name());
        qActor.bindValue(2, a.role());
        qActor.bindValue(3, a.number()); // Saved jersey number

        if (!qActor.exec())
        {
            return false;
        }

        // 2. Create Link
        qLink.bindValue(0, eventId.toHex());
        qLink.bindValue(1, actorHexId);

        if (!qLink.exec())
        {
            return false;
        }
    }

    return true;
}

bool EventDatabaseManager::deleteEvent(const UniqueId &eventId)
{
    QMutexLocker locker(&m_mutex);
    QSqlQuery query;
    query.prepare("DELETE FROM events WHERE event_id = ?");
    query.addBindValue(eventId.toHex());

    if (query.exec())
    {
        return true; // SQLite foreign keys handle cascading to children and actor links
    } else {
        emit errorOccurred("Delete Event Failed: " + query.lastError().text());
        return false;
    }
}

QList<QSharedPointer<Event>> EventDatabaseManager::getAllRootEvents() const
{
    // Reconstruct the Hierarchy from the flat database table
    QMap<QString, QSharedPointer<Event>> eventMap;
    QMap<QString, QString> parentMap; // ChildID -> ParentID

    QSqlQuery query("SELECT * FROM events");

    // PASS 1: Instantiate all objects
    while(query.next())
    {
        QString hexId = query.value("event_id").toString();
        QString type = query.value("type").toString();
        QString pid = query.value("parent_id").toString();

        QSharedPointer<Event> evt;
        if (type == "Atomic") {
            evt = QSharedPointer<AtomicEvent>::create();
        } else {
            evt = QSharedPointer<CompoundEvent>::create();
        }

        UniqueId evtId = UniqueId::fromString(hexId);

        // Populate Base Properties
        evt->setId(evtId);
        evt->setName(query.value("name").toString());
        evt->setDescription(query.value("description").toString());
        evt->setTimeRange(
            QDateTime::fromString(query.value("start_time").toString(), Qt::ISODate),
            QDateTime::fromString(query.value("end_time").toString(), Qt::ISODate)
        );

        evt->setConcept(query.value("concept").toString());
        evt->setMediaPath(query.value("media_path").toString());

        // Populate Location
        evt->setLocation(Location(
            query.value("loc_name").toString(),
            query.value("loc_city").toString(),
            query.value("loc_street").toString(),
            query.value("loc_road").toString(),
            query.value("loc_number").toString(),
            query.value("loc_country").toString()
        ));

        // Populate Actors
        QList<Actor> actors = this->getActorsForEvent(evtId);
        for(const auto &a : qAsConst(actors)) evt->addActor(a);

        eventMap.insert(hexId, evt);
        if (!pid.isEmpty()) parentMap.insert(hexId, pid);
    }

    // PASS 2: Link Children to Parents
    QList<QSharedPointer<Event>> roots;

    for (auto it = eventMap.begin(); it != eventMap.end(); ++it)
    {
        QString currentId = it.key();
        QSharedPointer<Event> currentEvt = it.value();

        if (parentMap.contains(currentId))
        {
            QString parentId = parentMap.value(currentId);

            if (eventMap.contains(parentId))
            {
                // We know parent must be Compound if data integrity is maintained
                auto parent = eventMap.value(parentId).staticCast<CompoundEvent>();

                if (parent) {
                    parent->addSubEvent(currentEvt);
                }
            }
        }
        else
        {
            // No parent map entry = Root Event
            roots.append(currentEvt);
        }
    }

    return roots;
}

QList<QSharedPointer<Event>> EventDatabaseManager::findEventsByHash(const QByteArray &hash, QCryptographicHash::Algorithm algo) const
{
    QList<QSharedPointer<Event>> results;
    QString hexHash = QString::fromUtf8(hash.toHex());

    // Determine the offset in the Hex string based on our layout
    int offset = 0;
    if (algo == QCryptographicHash::Md4) offset = 37; // SQL SUBSTR is 1-indexed
    else if (algo == QCryptographicHash::Md5) offset = 69;
    else return results;

    QSqlQuery query;
    // SUBSTR(column, start, length)
    query.prepare(QString("SELECT event_id FROM events WHERE SUBSTR(event_id, %1, 32) = ?").arg(offset));
    query.addBindValue(hexHash);

    if (query.exec())
    {
        while (query.next())
        {
            // Re-use our existing loader logic
            QString hexId = query.value(0).toString();
            // You could fetch the full object here or just return IDs
            // For this example, we'll assume a helper fetchEvent(hexId) exists
            results.append(this->getEventById(hexId));
        }
    }

    return results;
}

QList<QSharedPointer<Event>> EventDatabaseManager::findEventsByCRC(quint16 crc) const
{
    QList<QSharedPointer<Event>> results;
    QString hexCrc = QString("%1").arg(crc, 4, 16, QChar('0')); // Ensure 4 chars, padded with 0

    QSqlQuery query;
    // CRC starts at Hex offset 33 (1-indexed for SQL)
    query.prepare("SELECT event_id FROM events WHERE SUBSTR(event_id, 33, 4) = ?");
    query.addBindValue(hexCrc);

    if (query.exec())
    {
        while (query.next())
        {
            results.append(this->getEventById(query.value(0).toString()));
        }
    }

    return results;
}

QSharedPointer<Event> EventDatabaseManager::getEventById(const QString &hexId) const
{
    // 1. Single Recursive CTE to fetch the whole tree
    QSqlQuery query;
    QString cteQuery =
        "WITH RECURSIVE event_tree AS ("
        "  SELECT * FROM events WHERE event_id = ? "
        "  UNION ALL "
        "  SELECT e.* FROM events e "
        "  JOIN event_tree et ON e.parent_id = et.event_id "
        ") SELECT * FROM event_tree";

    query.prepare(cteQuery);
    query.addBindValue(hexId);

    if (!query.exec()) return nullptr;

    QMap<QString, QSharedPointer<Event>> eventMap;
    QList<QPair<QString, QString>> relationships;
    QStringList allEventIds;

    // 2. Pass 1: Instantiate Objects
    while (query.next())
    {
        QString currentId = query.value("event_id").toString();
        QString parentId = query.value("parent_id").toString();
        QString mediaIdHex = query.value("linked_media_id").toString();
        QString type = query.value("type").toString();
        allEventIds << currentId;

        QSharedPointer<Event> evt;
        if (type == "Atomic") evt = QSharedPointer<AtomicEvent>::create();
        else if (type == "Compound") evt = QSharedPointer<CompoundEvent>::create();
        else continue;

        if (!mediaIdHex.isEmpty())
        {
            evt->setLinkedMediaId(UniqueId::fromString(mediaIdHex));
        }

        evt->setId(UniqueId::fromString(currentId));
        evt->setName(query.value("name").toString());
        evt->setDescription(query.value("description").toString());
        evt->setConcept(query.value("concept").toString());
        evt->setMediaPath(query.value("media_path").toString());

        evt->setTimeRange(
            QDateTime::fromString(query.value("start_time").toString(), Qt::ISODate),
            QDateTime::fromString(query.value("end_time").toString(), Qt::ISODate)
            );

        evt->setLocation(Location(
            query.value("loc_name").toString(),
            query.value("loc_city").toString(),
            query.value("loc_street").toString(),
            query.value("loc_road").toString(),
            query.value("loc_number").toString(),
            query.value("loc_country").toString()
        ));

        // --- Refactored Analytical Fields (Gemini Insights) ---
        evt->setPriorityLevel(query.value("priority_level").toInt());
        evt->setCategory(query.value("category").toString());
        evt->setExpertAnalysis(query.value("expert_analysis").toString());
        evt->setVerdict(query.value("verdict").toString());
        evt->setConfidence(query.value("confidence").toString());
        evt->setVttStatus(query.value("vtt_status").toString());

        eventMap.insert(currentId, evt);
        if (!parentId.isEmpty()) relationships.append({currentId, parentId});
    }

    if (eventMap.isEmpty()) return nullptr;

    // 3. Optimized Actor Fetch: One query for the entire tree
    // Using a JOIN to get all actors associated with any ID in our tree
    QSqlQuery actorQuery;
    QString actorSql = QString(
                           "SELECT ea.event_id, a.actor_id, a.name, a.role a.number "
                           "FROM event_actors ea "
                           "JOIN actors a ON ea.actor_id = a.actor_id "
                           "WHERE ea.event_id IN (%1)"
                           ).arg(QStringList(allEventIds).join("','"));

    if (actorQuery.exec(actorSql))
    {
        while (actorQuery.next())
        {
            QString eid = actorQuery.value("event_id").toString();

            if (eventMap.contains(eid))
            {
                eventMap[eid]->addActor(Actor(
                    UniqueId::fromString(actorQuery.value("actor_id").toString()),
                    actorQuery.value("name").toString(),
                    actorQuery.value("role").toString(),
                    actorQuery.value("number").toInt()
                    ));
            }
        }
    }

    // 4. Pass 2: Reconstruct Hierarchy in memory
    for (const auto &rel : relationships)
    {
        if (eventMap.contains(rel.second) && eventMap.contains(rel.first))
        {
            auto parent = eventMap.value(rel.second).staticCast<CompoundEvent>();

            if (parent) parent->addSubEvent(eventMap.value(rel.first));
        }
    }

    return eventMap.value(hexId);
}

QList<QSharedPointer<Event>> EventDatabaseManager::searchEvents(const QString &text) const
{
    QList<QSharedPointer<Event>> results;

    if (text.trimmed().isEmpty()) return results;

    QSqlQuery query;
    // FTS5 uses the MATCH operator. Adding '*' allows for prefix matching (e.g., "foot*" matches "football")
    QString searchPattern = text.trimmed() + "*";

    query.prepare("SELECT event_id FROM events_search WHERE events_search MATCH ? ORDER BY rank");
    query.addBindValue(searchPattern);

    if (query.exec())
    {
        while (query.next())
        {
            QString hexId = query.value(0).toString();
            // Reuse your optimized getEventById to fetch the full tree/object
            auto evt = this->getEventById(hexId);

            if (evt) results.append(evt);
        }
    }

    return results;
}

QList<QSharedPointer<Event>> EventDatabaseManager::getEventsForFullVideo(const UniqueId &fullVideoId) const
{
    QList<QSharedPointer<Event>> results;
    QSqlQuery query;

    // We join events to segments, and segments to full_videos
    QString sql =
        "SELECT e.event_id FROM events e "
        "JOIN video_segments vs ON e.linked_media_id = vs.segment_id "
        "WHERE vs.full_video_id = ?";

    query.prepare(sql);
    query.addBindValue(fullVideoId.toHex());

    if (query.exec())
    {
        while (query.next())
        {
            QString hexId = query.value(0).toString();
            // Fetch the full hierarchical event tree
            auto evt = this->getEventById(hexId);

            if (evt) results.append(evt);
        }
    }

    return results;
}

QList<QSharedPointer<Event>> EventDatabaseManager::getTimelineForVideo(const UniqueId &fullVideoId) const
{
    // 1. Get all events linked to this video's segments
    QList<QSharedPointer<Event>> timeline = this->getEventsForFullVideo(fullVideoId);

    // 2. Sort the list based on start_time
    // We use a lambda for custom sorting
    std::sort(timeline.begin(), timeline.end(),
              [](const QSharedPointer<Event> &a, const QSharedPointer<Event> &b) {
                  // Sort by startTime; if times are identical, fallback to ID for stable sorting
                  if (a->startTime() != b->startTime()) {
                      return a->startTime() < b->startTime();
                  }
                  return a->id().toHex() < b->id().toHex();
              });

    return timeline;
}

bool EventDatabaseManager::batchImportFromJson(const QJsonArray &eventArray)
{
    if (eventArray.isEmpty()) return true;

    QMutexLocker locker(&m_mutex);

    // 1. Start a single massive transaction
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction())
    {
        qCritical() << "Batch Import: Could not start transaction.";
        return false;
    }

    // 2. Process each event in the array
    for (const QJsonValue &value : eventArray)
    {
        if (!value.isObject()) continue;

        // Reconstruct the Event tree using your existing factory
        QSharedPointer<Event> event = Event::fromJson(value.toObject());

        if (event)
        {
            // Use the recursive save logic we already built
            // We pass UniqueId() as the parent for these root events
            if (!this->saveEventRecursive(event, UniqueId()))
            {
                db.rollback();
                emit errorOccurred("Batch Import Failed during recursive save.");
                return false;
            }
        }
    }

    // 3. Commit everything to disk at once
    if (db.commit())
    {
        qInfo() << "Batch Import: Successfully imported" << eventArray.size() << "root events.";

        return true;
    }

    db.rollback();

    return false;
}

bool EventDatabaseManager::exportMatchPackage(const QString &rootEventHexId, const QString &destinationDir)
{
    QDir dir(destinationDir);
    if (!dir.mkpath("media")) return false;

    // 1. Fetch the entire hierarchy using our optimized CTE
    auto rootEvent = this->getEventById(rootEventHexId);
    if (!rootEvent) return false;

    // 2. Generate the JSON Manifest
    // We reuse the toJson() method we built into the Event classes
    QJsonObject manifest;
    manifest["version"] = "1.0";
    manifest["export_date"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    manifest["data"] = rootEvent->toJson();

    QFile manifestFile(dir.filePath("manifest.json"));
    if (manifestFile.open(QIODevice::WriteOnly)) {
        manifestFile.write(QJsonDocument(manifest).toJson());
        manifestFile.close();
    }

    // 3. Gather and Copy Media Files
    // We'll perform a recursive scan of the tree to find all mediaPaths
    QStringList mediaToCopy;

    std::function<void(QSharedPointer<Event>)> collectMedia = [&](QSharedPointer<Event> e) {

        if (!e->mediaPath().isEmpty()) mediaToCopy << e->mediaPath();

        if (!e->isAtomic())
        {
            for (const auto &sub : e.staticCast<CompoundEvent>()->subEvents()) collectMedia(sub);
        }
    };

    collectMedia(rootEvent);

    for (const QString &path : qAsConst(mediaToCopy)) {
        QFileInfo info(path);
        QFile::copy(path, dir.filePath("media/" + info.fileName()));
    }

    return true;
}

bool EventDatabaseManager::importMatchPackage(const QString &packageDir)
{
    QDir dir(packageDir);
    if (!dir.exists("manifest.json")) {
        emit errorOccurred("Import failed: manifest.json not found in " + packageDir);
        return false;
    }

    // 1. Read and Parse the Manifest
    QFile file(dir.filePath("manifest.json"));
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject rootObj = doc.object();
    QJsonObject eventData = rootObj["data"].toObject();

    // 2. Reconstruct the Event Tree using our Model's Factory
    // This handles Atomic vs Compound automatically
    QSharedPointer<Event> importedRoot = Event::fromJson(eventData);

    if (!importedRoot)
    {
        emit errorOccurred("Import failed: Could not reconstruct event tree from JSON.");
        return false;
    }

    // 3. Fix Media Paths
    // Since the package might be on a different machine, we need to update
    // the media paths to point to the current absolute path of the /media folder.
    std::function<void(QSharedPointer<Event>)> fixPaths = [&](QSharedPointer<Event> e)
    {
        if (!e->mediaPath().isEmpty())
        {
            QFileInfo oldInfo(e->mediaPath());
            // Update to the new local absolute path
            e->setMediaPath(dir.absoluteFilePath("media/" + oldInfo.fileName()));
        }

        if (!e->isAtomic())
        {
            auto compound = e.staticCast<CompoundEvent>();
            for (const auto &sub : compound->subEvents()) fixPaths(sub);
        }
    };

    fixPaths(importedRoot);

    // 4. Save to Database
    // Because saveEvent handles the transaction and recursion, this is now simple.
    qInfo() << "Importing match tree:" << importedRoot->name();

    return this->saveEvent(importedRoot);
}

// Inside your DB Manager class
Actor EventDatabaseManager::getActor(const UniqueId &actorId)
{
    QSqlQuery query;

    query.prepare("SELECT * FROM actors WHERE actor_id = ?");

    query.addBindValue(actorId.toHex());

    // 4. Execute the query
    if (!query.exec())
    {
        qCritical() << "Database Error:" << query.lastError().text();
        return Actor();
    }

    if (query.next())
    {
        // Recover the ID from the DB string
        UniqueId dbId = UniqueId::fromString(query.value("actor_id").toString());
        QString name = query.value("name").toString();
        QString role = query.value("role").toString();
        qint32 number = query.value("number").toInt();

        // The constructor creates the Actor with the DB ID fixed.
        return Actor(dbId, name, role, number);
    }

    return Actor();
}

QList<Actor> EventDatabaseManager::getActorsForEvent(const UniqueId &eventId) const
{
    QList<Actor> list;
    QSqlQuery query;

    query.prepare("SELECT a.actor_id, a.name, a.role FROM actors a "
                  "JOIN event_actors ea ON a.actor_id = ea.actor_id "
                  "WHERE ea.event_id = ?");

    query.addBindValue(eventId.toHex());

    if (query.exec())
    {
        while(query.next())
        {
            UniqueId actorId = UniqueId::fromString(query.value(0).toString());

            list.append(Actor(
                actorId,
                query.value(1).toString(),
                query.value(2).toString(),
                query.value(3).toInt()
            ));
        }
    }

    return list;
}
