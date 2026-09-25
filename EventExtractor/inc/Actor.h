#ifndef ACTOR_H
#define ACTOR_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QMetaType>

#include <QExplicitlySharedDataPointer>

#include "UniqueId.h"
using UniqueId = utils::UniqueId;

class ActorData;

class Actor
{
        // 1. Enable Qt Meta-Type System without QObject inheritance
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        // 2. Define Properties (accessible via introspection/QML)
        Q_PROPERTY(utils::UniqueId id READ id WRITE setId)
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QString role READ role WRITE setRole)

    public:

        // Constructors

        /**
         * @brief Actor
         */
        Actor();

        /**
         * @brief Actor: When an Actor is fetched from the database, this consctrutor
         *              populates the ID first, which prevents any subsquent logic
         *              from triggering a new hash.
         * @param id
         * @param name
         * @param role
         */
        Actor(const UniqueId &id, const QString &name, const QString &role, const qint32 number);

        /**
         * @brief Actor
         * @param right
         */
        Actor(const Actor &right);

        /**
         * @brief operator =
         * @param right
         * @return
         */
        Actor &operator=(const Actor &right);

        /**
         * @brief ~Actor
         */
        virtual ~Actor();

        // Operators

        /**
         * @brief operator ==
         * @param other
         * @return
         */
        bool operator==(const Actor &right) const;

        /**
         * @brief operator !=
         * @param other
         * @return
         */
        bool operator!=(const Actor &right) const;

        /**
         * @brief id
         * @return
         */
        UniqueId id() const;

        /**
         * @brief setId: If needed to change an ID manually after the object is created
         * @param newId
         */
        void setId(const UniqueId &newId);

        /**
         * @brief name
         * @return
         */
        QString name() const;

        /**
         * @brief setName
         * @param newName
         */
        void setName(const QString &newName);

        /**
         * @brief role
         * @return
         */
        QString role() const;

        /**
         * @brief setRole
         * @param role
         */
        void setRole(const QString &role);

        /**
         * @brief number
         * @return
         */
        qint32 number() const;

        /**
         * @brief setNumber
         * @param newNumber
         */
        void setNumber(qint32 newNumber);

        /**
         * @brief fromString
         * @param raw
         * @return
         */
        static Actor fromString(const QString &raw);

        // Serialization

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
        static Actor fromJson(const QJsonObject &json);

    private:

        QExplicitlySharedDataPointer<ActorData> sharedDataP;
};

// 3. Declare as a MetaType so it can be wrapped in QVariant
Q_DECLARE_METATYPE(Actor)

#endif // ACTOR_H
