
#include "inc/ExtractorUtils.h"
#include "inc/LogUtils.h"

#include "inc/EventModel.h"

// --- Event Base ---

Event::Event(QObject *parent) :
    QObject(parent),
    m_duration(0.0),
    m_priorityLevel(0)
{
    // TODO
}

UniqueId Event::id() const
{
    return this->m_id;
}

void Event::setId(const UniqueId &newId)
{
    if (this->m_id == newId) return;

    this->m_id = newId;

    emit dataChanged();
}

void Event::resetId()
{
    this->m_id = UniqueId(); // Clear it

    this->refreshId();       // Re-calculate based on current name/time

    emit dataChanged();
}

QString Event::name() const
{
    return this->m_name;
}

void Event::setName(const QString &newName)
{
    if (this->m_name == newName) return;

    this->m_name = newName;

    emit dataChanged();

    this->refreshId();
}

QString Event::description() const
{
    return this->m_description;
}

void Event::setDescription(const QString &newDesc)
{
    if (newDesc == this->m_description) return;

    this->m_description = newDesc;

    emit dataChanged();
}

QString Event::concept() const
{
    return this->m_concept;
}

void Event::setConcept(const QString &newConcept)
{
    if (newConcept == this->m_concept) return;

    this->m_concept = newConcept;

    emit dataChanged();

    this->refreshId();
}

QDateTime Event::startTime() const
{
    return this->m_startTime;
}

QDateTime Event::endTime() const
{
    return this->m_endTime;
}

void Event::setTimeRange(const QDateTime &newStart, const QDateTime &newEnd)
{
    if (m_startTime == newStart && this->m_endTime == newEnd) return;

    this->m_startTime = newStart;
    this->m_endTime = newEnd;

    double oldDuration = this->m_duration;

    if (this->m_startTime.isValid() && this->m_endTime.isValid())
    {
        this->m_duration = qAbs(this->m_startTime.msecsTo(m_endTime) / 1000.0);
    } else {
        this->m_duration = 0.0;
    }

    emit dataChanged();

    if (!qFuzzyCompare(oldDuration, this->m_duration))
    {
        emit durationChanged();

        this->refreshId();
    }
}

double Event::duration() const
{
    return this->m_duration;
}

int Event::priorityLevel() const
{
    return this->m_priorityLevel;
}
void Event::setPriorityLevel(qint32 level)
{
    if (level == this->m_priorityLevel) return;

    this->m_priorityLevel = level;

    emit dataChanged();
}

QString Event::category() const
{
    return this->m_category;
}

void Event::setCategory(const QString &cat)
{
    if (cat == this->m_category) return;

    this->m_category = cat;

    emit dataChanged();
}

QString Event::expertAnalysis() const
{
    return this->m_expertAnalysis;
}

void Event::setExpertAnalysis(const QString &analysis)
{
    if (analysis == this->m_expertAnalysis) return;

    this->m_expertAnalysis = analysis;

    emit dataChanged();
}

QString Event::verdict() const
{
    return this->m_verdict;
}

void Event::setVerdict(const QString &verdict)
{
    if (verdict == this->m_verdict) return;

    this->m_verdict = verdict;

    emit dataChanged();
}

QString Event::vttSource() const
{
    return this->m_vttSource;
}

void Event::setVttSource(const QString &source)
{
    if (source == this->m_vttSource) return;

    this->m_vttSource = source;

    emit dataChanged();
}

QString Event::vttStatus() const
{
    return this->m_vttStatus;
}

void Event::setVttStatus(const QString &status)
{
    if (status == this->m_vttStatus) return;

    this->m_vttStatus = status;

    emit dataChanged();
}

QString Event::confidence() const
{
    return this->m_confidence;
}

void Event::setConfidence(const QString &conf)
{
    if (conf == this->m_confidence) return;

    this->m_confidence = conf;

    emit dataChanged();
}

QString Event::mediaPath() const
{
    return this->m_mediaPath;
}

void Event::setMediaPath(const QString &newPath)
{
    if (newPath == this->m_mediaPath) return;

    this->m_mediaPath = newPath;

    emit dataChanged();
}

Location Event::location() const
{
    return this->m_location;
}

void Event::setLocation(const Location &newLoc)
{
    if (newLoc == this->m_location) return;

    this->m_location = newLoc;

    emit dataChanged();
}

QHash<QString, Actor> Event::actors() const
{
    return this->m_actors;
}

UniqueId Event::linkedMediaId() const
{
    return m_linkedMediaId;
}

void Event::setLinkedMediaId(const UniqueId &id)
{
    if (this->m_linkedMediaId == id) return;

    this->m_linkedMediaId = id;

    emit dataChanged();
}

QString Event::breadcrumb(const QString &separator) const
{
    QStringList path;
    const Event *current = this;

    // Traverse up the QObject parent-child tree
    while (current)
    {
        path.prepend(current->name());

        // Use qobject_cast to safely get the Event pointer of the parent
        current = qobject_cast<const Event*>(current->parent());
    }

    return path.join(separator);
}

QSharedPointer<Event> Event::fromJson(const QJsonObject &json)
{
    QString typeStr = json["type"].toString();
    QSharedPointer<Event> event;

    // Gemini type "Atomic (eva)" matches our AtomicEvent
    if (typeStr.startsWith("Atomic"))
    {
        event = QSharedPointer<AtomicEvent>::create();

    } else if (typeStr.startsWith("Compound")) {

        event = QSharedPointer<CompoundEvent>::create();
    } else {
        return nullptr;
    }

    event->loadBaseFromJson(json);

    return event;
}

QJsonObject Event::toJson() const
{
    QJsonObject obj;
    // Core Base Data
    obj["id"] = this->m_id.toHex();
    obj["description"] = this->m_description;
    obj["start_time"] = this->m_startTime.toString(Qt::ISODate);
    obj["end_time"] = this->m_endTime.toString(Qt::ISODate);
    obj["duration"] = this->m_duration;

    // New Analytical Data from JSON_response
    obj["priority_level"] = m_priorityLevel;
    obj["vtt_source"] = m_vttSource;
    obj["category"] = m_category;
    obj["vtt_status"] = m_vttStatus;
    obj["expert_analysis"] = m_expertAnalysis;
    obj["verdict"] = m_verdict;
    obj["confidence"] = m_confidence;

    // Actors / Players
    QJsonArray actorsArr;

    for (const auto &actor : this->m_actors) actorsArr.append(actor.toJson());

    obj["actors"] = actorsArr;

    return obj;
}

void Event::addActor(const Actor &actor)
{
    QString actorHex = actor.id().toHex();

    // Basic deduplication
    if (this->m_actors.contains(actorHex) &&
        this->m_actors.value(actorHex) == actor)
    {
        return;
    }

    // O(1) Insertion
    this->m_actors.insert(actor.id().toHex(), actor);

    emit dataChanged();
}

void Event::clearActors()
{
    if (m_actors.isEmpty()) return;

    this->m_actors.clear();

    emit dataChanged();
}

void Event::loadBaseFromJson(const QJsonObject &json)
{
    // 1. Identifiers & Basic Text
    // Mapping Gemini Keys to Class Members
    if (json.contains(QLatin1String("id"))) {
        this->setId(UniqueId::fromString(json.value(QLatin1String("id")).toString()));
    }

    this->setDescription(json.value(QLatin1String("description")).toString());
    this->setConcept(json.value(QLatin1String("concept")).toString());
    this->setMediaPath(json.value(QLatin1String("media_path")).toString());

    // 2. Timestamps
    // Handles standard ISO date-time if absolute, or verify validity
    const QString startStr = json.value(QLatin1String("start_time")).toString();
    const QString endStr   = json.value(QLatin1String("end_time")).toString();

    QDateTime start = QDateTime::fromString(startStr, Qt::ISODate);
    QDateTime end   = QDateTime::fromString(endStr, Qt::ISODate);

    // If Gemini emits timestamps instead of ISO dates (e.g., "HH:mm:ss" or "mm:ss"):
    // start = parseTimestampRelativeToBase(startStr);
    // end   = parseTimestampRelativeToBase(endStr);

    this->setTimeRange(start, end);

    // 3. Location Object
    const QJsonValue locVal = json.value(QLatin1String("location"));
    if (locVal.isObject()) {
        this->setLocation(Location::fromJson(locVal.toObject()));
    }

    // 4. Priority and Category
    this->setPriorityLevel(json.value(QLatin1String("priority_level")).toInt(0));
    this->setCategory(json.value(QLatin1String("category")).toString());

    // 5. AI Insights
    this->setExpertAnalysis(json.value(QLatin1String("expert_analysis")).toString());
    this->setVerdict(json.value(QLatin1String("verdict")).toString());
    this->setConfidence(json.value(QLatin1String("confidence")).toString());
    this->setVttSource(json.value(QLatin1String("vtt_source")).toString());
    this->setVttStatus(json.value(QLatin1String("vtt_status")).toString());

    // 6. Players / Actors
    this->clearActors();
    const QJsonValue playersVal = json.value(QLatin1String("players"));
    if (playersVal.isArray()) {
        const QJsonArray playersArray = playersVal.toArray();
        for (const QJsonValue &val : playersArray) {
            if (val.isObject()) {
                // If the array contains JSON objects
                this->addActor(Actor::fromJson(val.toObject()));
            } else if (val.isString()) {
                // If the array contains formatted strings (e.g., "[#10, Brazil]")
                this->addActor(Actor::fromString(val.toString()));
            }
        }
    }
}

void Event::copyBaseTo(QSharedPointer<Event> target, bool resetIds) const
{
    target->setName(this->m_name);
    target->setDescription(this->m_description);
    target->setTimeRange(this->m_startTime, this->m_endTime);
    target->setConcept(this->m_concept);
    target->setMediaPath(this->m_mediaPath);
    target->setLocation(this->m_location);

    for (const auto &actor : this->m_actors)
    {
        // Actors are COW (Copy-on-Write), so a simple append is fine.
        // Their internal data will detach if modified.
        target->addActor(actor);
    }

    if (resetIds)
    {
        // Clear the ID so the refreshId() logic generates a new one
        // based on the properties (or a new random UUID).
        target->setId(UniqueId());
        // We'll call target->refreshId() in the subclasses
    } else {
        target->setId(this->m_id);
    }
}

void Event::refreshId()
{
    // If the ID was already set (e.g., loaded from DB),
    // we usually don't want to change it just because a name was edited.
    // Use this check if you want the ID to be "Permid" (Permanent ID)
    if (!this->m_id.isEmpty()) return;

    // Generate a seed based on core properties
    // We use name + concept + ISO start time to make it unique but reproducible
    QString seedSource = this->m_name + this->m_concept + this->m_startTime.toString(Qt::ISODate);

    if (seedSource.isEmpty()) return;

    QByteArray eventSeed = ExtractorUtils::generateContentSeed(seedSource, QCryptographicHash::Md4);

    // This calls the UniqueId(QByteArray) constructor which runs the hashing logic
    this->m_id = UniqueId(eventSeed);
}

// --- Atomic Event ---

AtomicEvent::AtomicEvent(QObject *parent) :
    Event(parent)
{
    // TODO
}

bool AtomicEvent::isAtomic() const
{
    return true;
}

QJsonObject AtomicEvent::toJson() const
{
    QJsonObject obj = Event::toJson();
    obj["type"] = "Atomic";

    return obj;
}

QSharedPointer<Event> AtomicEvent::clone(bool resetIds) const
{
    auto newEvent = QSharedPointer<AtomicEvent>::create();

    this->copyBaseTo(newEvent, resetIds);

    if (resetIds) newEvent->refreshId();

    return newEvent;
}

// --- Compound Event ---

CompoundEvent::CompoundEvent(QObject *parent) :
    Event(parent)
{
    // TODO
}

bool CompoundEvent::isAtomic() const
{
    return false;
}

QJsonObject CompoundEvent::toJson() const
{
    QJsonObject obj = Event::toJson();
    obj["type"] = "Compound";

    QJsonArray subArr;
    for (const auto &child :this-> m_subEvents)
    {
        subArr.append(child->toJson());
    }

    obj["sub_events"] = subArr;

    return obj;
}

QSharedPointer<Event> CompoundEvent::clone(bool resetIds) const
{
    auto newEvent = QSharedPointer<CompoundEvent>::create();
    this->copyBaseTo(newEvent, resetIds);

    for (const auto &child : this->m_subEvents)
    {
        // Recursively clone every child
        newEvent->addSubEvent(child->clone(resetIds));
    }

    if (resetIds) newEvent->refreshId();

    return newEvent;
}

void CompoundEvent::addSubEvent(const QSharedPointer<Event> &event)
{
    if (event)
    {
        this->m_subEvents.append(event);
        // Optional: Re-parenting QObject to maintain hierarchy tree in memory tool
        event->setParent(this);

        this->updateDurationFromChildren();

        emit dataChanged();
    }
}

QList<QSharedPointer<Event>> CompoundEvent::subEvents() const
{
    return this->m_subEvents;
}

void CompoundEvent::updateDurationFromChildren()
{
    const QList<QSharedPointer<Event>> &subEvents = this->m_subEvents;

    if (subEvents.isEmpty())
    {
        this->setTimeRange(QDateTime(), QDateTime());
        return;
    }

    QDateTime earliest;
    QDateTime latest;

    foreach (const QSharedPointer<Event> &sub, this->m_subEvents)
    {
        if (!sub->startTime().isValid() || !sub->endTime().isValid()) continue;

        if (earliest.isNull() || sub->startTime() < earliest)
            earliest = sub->startTime();

        if (latest.isNull() || sub->endTime() > latest)
            latest = sub->endTime();
    }

    if (earliest.isValid() && latest.isValid())
        this->setTimeRange(earliest, latest);
}
