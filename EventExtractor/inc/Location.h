#ifndef LOCATION_H
#define LOCATION_H

#include <QObject>
#include <QJsonObject>
#include <QMetaType>
#include <QString>

class Location
{
        // 1. Enable Qt Meta-Type System
        Q_GADGET

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

        // 2. Define Properties
        Q_PROPERTY(QString name READ name WRITE setName)
        Q_PROPERTY(QString city READ city WRITE setCity)
        Q_PROPERTY(QString country READ country WRITE setCountry)

    public:
        // Constructors

        /**
         * @brief Location
         */
        Location();

        /**
         * @brief Location
         * @param name
         * @param city
         * @param street
         * @param road
         * @param number
         * @param country
         */
        Location(const QString &name,
                 const QString &city,
                 const QString &street,
                 const QString &road,
                 const QString &number,
                 const QString &country);

        // Standard Copy/Move (Compiler defaults are suitable)
        Location(const Location &other) = default;
        Location &operator=(const Location &other) = default;
        ~Location() = default;

        // Operators

        /**
         * @brief operator ==
         * @param other
         * @return
         */
        bool operator==(const Location &other) const;

        /**
         * @brief operator !=
         * @param other
         * @return
         */
        bool operator!=(const Location &other) const;

        // Getters

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
         * @brief street
         * @return
         */
        QString street() const;

        /**
         * @brief setStreet
         * @param newStreet
         */
        void setStreet(const QString &newStreet);

        /**
         * @brief road
         * @return
         */
        QString road() const;

        /**
         * @brief setRoad
         * @param newRoad
         */
        void setRoad(const QString &newRoad);

        /**
         * @brief number
         * @return
         */
        QString number() const;

        /**
         * @brief setNumber
         * @param newNumber
         */
        void setNumber(const QString &newNumber);

        /**
         * @brief city
         * @return
         */
        QString city() const;

        /**
         * @brief setCity
         * @param newCity
         */
        void setCity(const QString &newCity);

        /**
         * @brief country
         * @return
         */
        QString country() const;

        /**
         * @brief setCountry
         * @param newCountry
         */
        void setCountry(const QString &newCountry);

        // Utilities

        /**
         * @brief toString
         * @return
         */
        QString toString() const;

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
        static Location fromJson(const QJsonObject &json);

    private:

        QString m_name;
        QString m_street;
        QString m_road;
        QString m_number;
        QString m_city;
        QString m_country;
};

// 3. Register with QVariant
Q_DECLARE_METATYPE(Location)

#endif // LOCATION_H
