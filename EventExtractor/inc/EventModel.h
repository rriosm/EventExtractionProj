#ifndef EVENTMODEL_H
#define EVENTMODEL_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QList>
#include <QSharedPointer> // We still use this for memory management
#include <QJsonObject>
#include <QJsonArray>

#include "UniqueId.h"
using namespace utils;

#include "inc/Actor.h"      // Assuming Actor is the Q_GADGET version
#include "inc/Location.h"   // Assuming Location is the Q_GADGET version

// --- Base Class: Event ---
class Event : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        // Properties for Reflection/QML
        Q_PROPERTY(UniqueId id READ id WRITE setId NOTIFY dataChanged)
        Q_PROPERTY(QString name READ name WRITE setName NOTIFY dataChanged)
        Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
        Q_PROPERTY(bool isAtomic READ isAtomic CONSTANT)

        Q_PROPERTY(qint32 priorityLevel READ priorityLevel WRITE setPriorityLevel NOTIFY dataChanged)
        Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY dataChanged)
        Q_PROPERTY(QString expertAnalysis READ expertAnalysis WRITE setExpertAnalysis NOTIFY dataChanged)
        Q_PROPERTY(QString verdict READ verdict WRITE setVerdict NOTIFY dataChanged)
        Q_PROPERTY(QString confidence READ confidence WRITE setConfidence NOTIFY dataChanged)
        Q_PROPERTY(QString vttStatus READ vttStatus WRITE setVttStatus NOTIFY dataChanged)

    public:
        /**
         * @brief Event
         * @param parent
         */
        explicit Event(QObject *parent = nullptr);

        /**
         * @brief ~Event
         */
        virtual ~Event() = default;

        // Factory Method (Returns Shared Pointer)

        // Getters

        /**
         * @brief id
         * @return
         */
        UniqueId id() const;

        /**
         * @brief setId
         * @param id
         */
        void setId(const UniqueId &id);

        /**
         * @brief resetId
         */
        void resetId();

        /**
         * @brief name
         * @return
         */
        QString name() const;

        /**
         * @brief setName
         * @param name
         */
        void setName(const QString &name);

        /**
         * @brief description
         * @return
         */
        QString description() const;

        /**
         * @brief setDescription
         * @param desc
         */
        void setDescription(const QString &desc);

        /**
         * @brief concept
         * @return
         */
        QString concept() const;

        /**
         * @brief setConcept
         * @param newConcept
         */
        void setConcept(const QString &newConcept);

        /**
         * @brief startTime
         * @return
         */
        QDateTime startTime() const;

        /**
         * @brief endTime
         * @return
         */
        QDateTime endTime() const;

        /**
         * @brief setTimeRange
         * @param start
         * @param end
         */
        void setTimeRange(const QDateTime &start, const QDateTime &end);

        /**
         * @brief duration
         * @return
         */
        double duration() const;

        // --- AI Analytical ---

        /**
         * @brief priorityLevel
         * @return
         */
        qint32 priorityLevel() const;

        /**
         * @brief setPriorityLevel
         * @param level
         */
        void setPriorityLevel(qint32 level);

        /**
         * @brief category
         * @return
         */
        QString category() const;

        /**
         * @brief setCategory
         * @param cat
         */
        void setCategory(const QString &cat);

        /**
         * @brief expertAnalysis
         * @return
         */
        QString expertAnalysis() const;

        /**
         * @brief setExpertAnalysis
         * @param analysis
         */
        void setExpertAnalysis(const QString &analysis);

        /**
         * @brief verdict
         * @return
         */
        QString verdict() const;

        /**
         * @brief setVerdict
         * @param verdict
         */
        void setVerdict(const QString &verdict);

        /**
         * @brief confidence
         * @return
         */
        QString confidence() const;

        /**
         * @brief setConfidence
         * @param conf
         */
        void setConfidence(const QString &conf);

        /**
         * @brief vttSource
         * @return
         */
        QString vttSource() const;

        /**
         * @brief setVttSource
         * @param source
         */
        void setVttSource(const QString &source);

        /**
         * @brief vttStatus
         * @return
         */
        QString vttStatus() const;

        /**
         * @brief setVttStatus
         * @param status
         */
        void setVttStatus(const QString &status);

        /**
         * @brief mediaPath
         * @return
         */
        QString mediaPath() const;

        /**
         * @brief setMediaPath
         * @param newPath
         */
        void setMediaPath(const QString &newPath);

        /**
         * @brief location
         * @return
         */
        Location location() const;

        /**
         * @brief setLocation
         * @param newLoc
         */
        void setLocation(const Location &newLoc);

        /**
         * @brief actors
         * @return
         */
        QHash<QString, Actor> actors() const;

        /**
         * @brief linkedMediaId
         * @return
         */
        UniqueId linkedMediaId() const;

        /**
         * @brief setLinkedMediaId
         * @param id
         */
        void setLinkedMediaId(const UniqueId &id);

        /**
         * @brief breadcrumb: provide context in the UI, especially when dealing with
         *                    deep hierarchies of events
         * @param separator
         * @return
         */
        QString breadcrumb(const QString &separator) const;

        /**
         * @brief fromJson
         * @param json
         * @return
         */
        static QSharedPointer<Event> fromJson(const QJsonObject &json);

        /**
         * @brief toJson
         * @return
         */
        virtual QJsonObject toJson() const;

        /**
         * @brief clone
         * @param resetIds
         * @return
         */
        virtual QSharedPointer<Event> clone(bool resetIds = true) const = 0;

        // Pure Virtual function makes this abstract
        virtual bool isAtomic() const = 0;

        // Setters (Now Slots)
    public slots:

        /**
         * @brief addActor
         * @param actor
         */
        void addActor(const Actor &actor);

        /**
         * @brief clearActors
         */
        void clearActors();

    signals:

        /**
         * @brief dataChanged
         */
        void dataChanged();

        /**
         * @brief durationChanged
         */
        void durationChanged();

    protected:

        /**
         * @brief loadBaseFromJson
         * @param json
         */
        void loadBaseFromJson(const QJsonObject &json);

        /**
         * @brief copyBaseTo
         * @param target
         * @param resetIds
         */
        void copyBaseTo(QSharedPointer<Event> target, bool resetIds) const;

        /**
         * @brief refreshId
         */
        void refreshId();

    protected:

        // Identification
        UniqueId m_id;            // Event Id
        UniqueId m_linkedMediaId; // Video segment Id

        // Informational
        QString m_name;
        QString m_description;
        QString m_concept;

        // Temporal
        mutable QDateTime m_startTime;
        mutable QDateTime m_endTime;
        double m_duration;

        // Experiential
        QString m_mediaPath;

        // Spatial & Social
        Location m_location;            // Q_GADGET (Stored by value)
        QHash<QString, Actor> m_actors; // Q_GADGET (Stored by value)

        // --- New Analytical Members ---
        qint32 m_priorityLevel;     // "priority_level": 7
        QString m_category;         // "category": "FOUL"
        QString m_expertAnalysis;   // "expert_analysis": "..."
        QString m_verdict;          // "verdict": "..."
        QString m_confidence;       // "confidence": "high"
        QString m_vttSource;        // "vtt_source": "..."
        QString m_vttStatus;        // "vtt_status": "confirmed"

};

// --- Derived Class: Atomic Event ---
class AtomicEvent : public Event
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

    public:

        /**
         * @brief AtomicEvent
         * @param parent
         */
        explicit AtomicEvent(QObject *parent = nullptr);

        /**
         * @brief isAtomic
         * @return
         */
        bool isAtomic() const override;

        /**
         * @brief toJson
         * @return
         */
        QJsonObject toJson() const override;

        /**
         * @brief clone
         * @param resetIds
         * @return
         */
        QSharedPointer<Event> clone(bool resetIds) const Q_DECL_OVERRIDE;
};

// --- Derived Class: Compound Event ---

class CompoundEvent : public Event
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

    public:

        /**
         * @brief CompoundEvent
         * @param parent
         */
        explicit CompoundEvent(QObject *parent = nullptr);

        /**
         * @brief isAtomic
         * @return
         */
        bool isAtomic() const override;

        /**
         * @brief toJson
         * @return
         */
        QJsonObject toJson() const override;

        /**
         * @brief clone
         * @param resetIds
         * @return
         */
        QSharedPointer<Event> clone(bool resetIds) const Q_DECL_OVERRIDE;

        // Child Management
        // We use QSharedPointer to ensure children stay alive even if parent is deleted

        /**
         * @brief addSubEvent
         * @param event
         */
        void addSubEvent(const QSharedPointer<Event> &event);

        /**
         * @brief subEvents
         * @return
         */
        QList<QSharedPointer<Event>> subEvents() const;

        /**
         * @brief updateDurationFromChildren
         */
        void updateDurationFromChildren();

    private:

        // Structural / Compositional
        QList<QSharedPointer<Event>> m_subEvents;
};

#endif // EVENTMODEL_H
