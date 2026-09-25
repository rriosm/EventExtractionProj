
#include <QSharedData>

#include "inc/Actor.h"

#include "inc/ExtractorUtils.h"

class ActorData: public QSharedData
{
    public:

        ActorData()
        {
            // TODO
        }

        ~ActorData() = default;

        ActorData(const ActorData &actorData) :
            QSharedData(actorData),
            m_id(actorData.m_id),
            m_name(actorData.m_name),
            m_role(actorData.m_role),
            m_number(actorData.m_number)
        {
            // TODO
        }

        bool operator==(const ActorData &other) const
        {
            return ( this->m_id == other.m_id &&
                    this->m_name == other.m_name &&
                    this->m_role == other.m_role &&
                    this->m_number == other.m_number);
        }

        UniqueId m_id;
        QString m_name;
        QString m_role;
        qint32 m_number;
};

void _updateInternalId(ActorData* data)
{
    if (!data->m_id.isEmpty()) return;

    if (data->m_name.isEmpty() || data->m_role.isEmpty()) return;

    QByteArray actorSeed = ExtractorUtils::generateContentSeed(data->m_name + data->m_role + QString::number(data->m_number),
                                                      QCryptographicHash::Md4);

    data->m_id = UniqueId(actorSeed);
}

Actor::Actor() :
    sharedDataP(new ActorData)
{
    // TODO
}

Actor::Actor(const UniqueId &id, const QString &name, const QString &role, const qint32 number) :
    sharedDataP(new ActorData)
{
    this->sharedDataP->m_id = id;
    this->sharedDataP->m_name = name;
    this->sharedDataP->m_role = role;
    this->sharedDataP->m_number = number;
}

Actor::~Actor()
{
    // TODO
}

Actor::Actor(const Actor &right) :
    sharedDataP(right.sharedDataP)
{
    // TODO
}

Actor &Actor::operator=(const Actor &right)
{
    if (this != &right)
    {
        this->sharedDataP = right.sharedDataP;
    }

    return *this;
}

bool Actor::operator==(const Actor &right) const
{
    if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

    return (*this->sharedDataP == *right.sharedDataP);
}

bool Actor::operator!=(const Actor &other) const
{
    return !(*this == other);
}

UniqueId Actor::id() const
{
    return this->sharedDataP->m_id;
}

void Actor::setId(const UniqueId &newId)
{
    if (newId == this->sharedDataP->m_id) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_id = newId;
}

QString Actor::name() const
{
    return this->sharedDataP->m_name;
}

void Actor::setName(const QString &newName)
{
    if (this->sharedDataP->m_name == newName) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_name = newName;

    if (this->sharedDataP->m_role.isEmpty()) return;

    _updateInternalId(this->sharedDataP.data());
}

QString Actor::role() const
{
    return this->sharedDataP->m_role;
}

void Actor::setRole(const QString &newRole)
{
    if (this->sharedDataP->m_role == newRole) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_role = newRole;

    if (this->sharedDataP->m_name.isEmpty()) return;

    _updateInternalId(this->sharedDataP.data());
}

qint32 Actor::number() const
{
    return this->sharedDataP->m_number;
}

void Actor::setNumber(qint32 newNumber)
{
    if (newNumber == this->sharedDataP->m_number) return;

    this->sharedDataP.detach();

    this->sharedDataP->m_number = newNumber;

    if (this->sharedDataP->m_name.isEmpty()) return;

    _updateInternalId(this->sharedDataP.data());
}

Actor Actor::fromString(const QString &raw)
{
    // Regex matches: [#<number>, <name> (<role/color>)]
    // Captures: 1: number, 2: name, 3: role
    static QRegularExpression re(R"(\[#(\d+),\s*([^\(]+?)\s*(?:\((.*)\))?\])");

    QRegularExpressionMatch match = re.match(raw);

    Actor actor;

    if (match.hasMatch())
    {
        actor.setNumber(match.captured(1).toInt());
        actor.setName(match.captured(2).trimmed());
        actor.setRole(match.captured(3).isEmpty() ? "Player" : match.captured(3).trimmed());
    } else {
        // Fallback for simple names
        actor.setName(raw.simplified());
        actor.setRole("Unknown");
    }

    return actor;
}

// --- Serialization ---
QJsonObject Actor::toJson() const
{
    QJsonObject obj;
    obj["id"] = this->sharedDataP->m_id.toHex();
    obj["name"] = this->sharedDataP->m_name;
    obj["role"] = this->sharedDataP->m_role;
    obj["number"] = number();

    return obj;
}

Actor Actor::fromJson(const QJsonObject &json)
{
    return Actor(
        UniqueId::fromString(json["id"].toString()),
        json["name"].toString(),
        json["role"].toString(),
        json["number"].toInt()
    );
}

static int registerActorMetatype()
{
    return qRegisterMetaType<Actor>("Actor");
}

static int dummyActor = registerActorMetatype();
