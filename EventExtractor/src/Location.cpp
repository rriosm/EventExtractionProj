
#include "inc/Location.h"

Location::Location() :
    m_name(QString()),
    m_street(QString()),
    m_road(QString()),
    m_number(QString()),
    m_city(QString()),
    m_country(QString())
{
    // TODO
}

Location::Location(const QString &name,
                   const QString &city,
                   const QString &street,
                   const QString &road,
                   const QString &number,
                   const QString &country)
    : m_name(name),
      m_street(street),
      m_road(road),
      m_number(number),
      m_city(city),
      m_country(country)
{
    // TODO
}

// --- Operators ---
bool Location::operator==(const Location &other) const
{
    return ( (this->m_name == other.m_name) &&
             (this->m_street == other.m_street) &&
             (this->m_road == other.m_road) &&
             (this->m_number == other.m_number) &&
             (this->m_city == other.m_city) &&
             (this->m_country == other.m_country) );
}

bool Location::operator!=(const Location &other) const
{
    return !(*this == other);
}

// --- Getters ---
QString Location::name() const
{
    return this->m_name;
}

void Location::setName(const QString &newName)
{
    if (this->m_name == newName) return;

    this->m_name = newName;
}

QString Location::street() const
{
    return m_street;
}

void Location::setStreet(const QString &newStreet)
{
    if (this->m_street == newStreet) return;

    this->m_street = newStreet;
}

QString Location::road() const
{
    return m_road;
}

void Location::setRoad(const QString &newRoad)
{
    if (this->m_road == newRoad) return;

    this->m_road = newRoad;
}

QString Location::number() const
{
    return this->m_number;
}

void Location::setNumber(const QString &newNumber)
{
    if (this->m_number == newNumber) return;

    this->m_number = newNumber;
}

QString Location::city() const
{
    return m_city;
}

void Location::setCity(const QString &newCity)
{
    if (this->m_city == newCity) return;

    this->m_city = newCity;
}

QString Location::country() const
{
    return this->m_country;
}

void Location::setCountry(const QString &newCountry)
{
    if (this->m_country == newCountry) return;

    this->m_country = newCountry;
}

// --- Utilities ---
QString Location::toString() const
{
    if ( this->m_name.isEmpty() &&
         this->m_city.isEmpty() )
        return QString();

    QStringList addressParts;

    if (!this->m_name.isEmpty()) addressParts << this->m_name;

    if (!this->m_number.isEmpty()) addressParts << this->m_number;
    if (!this->m_street.isEmpty()) addressParts << this->m_street;
    if (!this->m_road.isEmpty()) addressParts << this->m_road;
    if (!this->m_city.isEmpty()) addressParts << this->m_city;

    QString result = addressParts.join(", ");

    // Country/Affiliation context (e.g., "Brazil (Yellow)")
    if ( ! this->m_country.isEmpty() )
    {
        result += " (" + m_country + ")";
    }

    return result;
}

// --- Serialization ---
QJsonObject Location::toJson() const
{
    QJsonObject obj;
    obj["name"] = m_name;
    obj["street"] = m_street;
    obj["road"] = m_road;
    obj["number"] = m_number;
    obj["city"] = m_city;
    obj["country"] = m_country;

    return obj;
}

Location Location::fromJson(const QJsonObject &json)
{
    // Using explicit setters to ensure no fields are missed during reconstruction
    Location loc;
    loc.setName(json["name"].toString());
    loc.setStreet(json["street"].toString());
    loc.setRoad(json["road"].toString());
    loc.setNumber(json["number"].toString());
    loc.setCity(json["city"].toString());
    loc.setCountry(json["country"].toString());
    return loc;
}
