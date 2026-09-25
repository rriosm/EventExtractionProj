
#include "UniqueId.h"

#include "NamespaceDefs.h"

#include "Logger.h"
using namespace logger;

QT_BEGIN_NAMESPACE
#include <QSharedData>
#include <QHash>
#include <QCryptographicHash>
QT_END_NAMESPACE

namespace utils {

    class UniqueIdData : public QSharedData
    {
        public:

            UniqueIdData() :
                QSharedData(),
                m_uuid(QUuid::createUuid())
            {
                this->m_uniqueId.clear();
                this->m_md4Hash.clear();
                this->m_md5Hash.clear();
                this->m_crc16 = 0;
            }

            UniqueIdData(const UniqueIdData &uniqueData) :
                QSharedData(uniqueData),
                m_uuid(uniqueData.m_uuid),
                m_uniqueId(uniqueData.m_uniqueId),
                m_md4Hash(uniqueData.m_md4Hash),
                m_md5Hash(uniqueData.m_md5Hash),
                m_crc16(uniqueData.m_crc16)
            {
                // TODO
            }

            bool operator==(const UniqueIdData &other) const
            {
                return ( this->m_uuid == other.m_uuid &&
                        this->m_uniqueId == other.m_uniqueId &&
                        this->m_md4Hash == other.m_md4Hash &&
                        this->m_md5Hash == other.m_md5Hash &&
                        this->m_crc16 == other.m_crc16 );
            }

            QUuid m_uuid;

            QByteArray m_uniqueId;

            QByteArray m_md4Hash;

            QByteArray m_md5Hash;

            quint16 m_crc16;
    };

    UniqueId::UniqueId(const QByteArray &seed) :
        sharedDataP(new UniqueIdData)
    {
        if (!seed.isEmpty())
        {
            this->id(seed);
        }
    }

    UniqueId::~UniqueId()
    {
        // TODO
    }

    UniqueId::UniqueId(const UniqueId &rhs) :
        sharedDataP(rhs.sharedDataP)
    {
        // TODO
    }

    /**
         * @brief operator =
         * @param right
         * @return
         */
    UniqueId & UniqueId::operator=(const UniqueId &right)
    {
        if (this != &right)
        {
            this->sharedDataP.operator=(right.sharedDataP);
        }

        return *this;
    }

    /**
         * @brief operator ==
         * @param right
         * @return
         */
    bool UniqueId::operator ==(const UniqueId &right) const
    {
        if (this->sharedDataP == right.sharedDataP) return true; // Same memory object

        return (*this->sharedDataP == *right.sharedDataP); // Deep value compare
    }

    /**
         * @brief operator !=
         * @param right
         * @return
         */
    bool UniqueId::operator !=(const UniqueId &right) const
    {
        return !(*this == right);
    }

    /**
         * @brief operator ==
         * @param ptr
         * @return
         */
    bool UniqueId::operator==(const UniqueId *ptr) const
    {
        return (this == ptr);
    }

    /**
         * @brief operator !=
         * @param ptr
         * @return
         */
    bool UniqueId::operator!=(const UniqueId *ptr) const
    {
        return (this != ptr);
    }

    bool UniqueId::operator<(const UniqueId &right) const
    {
        if (this->sharedDataP == right.sharedDataP) return false;

        return (this->sharedDataP->m_uniqueId < right.sharedDataP->m_uniqueId);
    }

    void UniqueId::detach()
    {
        this->sharedDataP.detach();
    }

    quint16 UniqueId::checksumCRC16() const
    {
        return this->sharedDataP->m_crc16;
    }

    const QByteArray & UniqueId::checksumMD4() const
    {
        return this->sharedDataP->m_md4Hash;
    }

    const QByteArray & UniqueId::checksumMD5() const
    {
        return this->sharedDataP->m_md5Hash;
    }

    QByteArray UniqueId::id(const QByteArray &seed)
    {
        if (this->sharedDataP->m_uniqueId.isEmpty())
        {
            if (!seed.isNull() && !seed.isEmpty())
            {
                this->detach();

                // 1. Generate Hashes and Checksum
                this->sharedDataP->m_md4Hash = QCryptographicHash::hash(seed, QCryptographicHash::Md4);
                this->sharedDataP->m_md5Hash = QCryptographicHash::hash(seed, QCryptographicHash::Md5);
                this->sharedDataP->m_crc16 = qChecksum(seed.constData(),
                                                       seed.size());

                this->sharedDataP->m_uniqueId.clear();

                // 2. Build the binary blob
                // UUID: 16 bytes raw
                this->sharedDataP->m_uniqueId.append(this->sharedDataP->m_uuid.toRfc4122());

                // CRC16: 2 bytes (Big Endian)
                this->sharedDataP->m_uniqueId.append(static_cast<char>((this->sharedDataP->m_crc16 >> 8) & 0xFF));
                this->sharedDataP->m_uniqueId.append(static_cast<char>(this->sharedDataP->m_crc16 & 0xFF));

                // Hashes: 16 bytes each
                this->sharedDataP->m_uniqueId.append(this->sharedDataP->m_md4Hash);
                this->sharedDataP->m_uniqueId.append(this->sharedDataP->m_md5Hash);
            }
        }

        return this->sharedDataP->m_uniqueId;
    }

    const QUuid & UniqueId::uuid() const
    {
        return this->sharedDataP->m_uuid;
    }

    bool UniqueId::isEmpty() const
    {
        return this->sharedDataP->m_uniqueId.isEmpty();
    }

    QJsonObject UniqueId::toJson() const
    {
        QJsonObject obj;

        obj["uuid"] = sharedDataP->m_uuid.toString();
        obj["uniqueId"] = QString::fromUtf8(sharedDataP->m_uniqueId.toBase64());
        obj["md4"] = QString::fromUtf8(sharedDataP->m_md4Hash.toHex());
        obj["md5"] = QString::fromUtf8(sharedDataP->m_md5Hash.toHex());
        obj["crc16"] = static_cast<int>(sharedDataP->m_crc16);

        return obj;
    }

    UniqueId UniqueId::fromJson(const QJsonObject &json)
    {
        // Create a new instance (this allocates a new UniqueIdData internally)
        UniqueId newId;

        if (json.isEmpty())
        {
            Logger::instance()->log(QtDebugMsg,
                                    QString("UniqueId::fromJson(const QJsonObject &json) - Attempted to create UniqueId from empty JSON object."),
                                    __FILE__,
                                    __LINE__,
                                    Q_FUNC_INFO);

            return newId;
        }

        // Access the internal data directly.
        // Since this is a fresh object, no detach() is needed yet.
        if (json.contains("uuid"))
        {
            newId.sharedDataP->m_uuid = QUuid(json["uuid"].toString());
        }

        if (json.contains("uniqueId"))
        {
            // IDs are stored as Base64 in JSON to ensure binary safety
            newId.sharedDataP->m_uniqueId = QByteArray::fromBase64(json["uniqueId"].toString().toUtf8());
        }

        if (json.contains("md4"))
        {
            newId.sharedDataP->m_md4Hash = QByteArray::fromHex(json["md4"].toString().toUtf8());
        }

        if (json.contains("md5"))
        {
            newId.sharedDataP->m_md5Hash = QByteArray::fromHex(json["md5"].toString().toUtf8());
        }

        if (json.contains("crc16"))
        {
            newId.sharedDataP->m_crc16 = static_cast<quint16>(json["crc16"].toInt());
        }

        return newId;
    }

    QString UniqueId::toString() const
    {
        return QString::fromUtf8(this->sharedDataP->m_uniqueId);
    }

    QString UniqueId::toHex() const
    {
        if (this->sharedDataP->m_uniqueId.isEmpty())
        {
            return QString();
        }

        return QString::fromUtf8(this->sharedDataP->m_uniqueId.toHex());
    }

    UniqueId UniqueId::fromString(const QString &str)
    {
        // 1. Database stores the Hex of the concatenated blob
        QByteArray data = QByteArray::fromHex(str.toUtf8());

        // Length calculation:
        // UUID String (16) + CRC (2) + MD4 (16) + MD5 (16) = 50 bytes
        if (data.size() < 50)
        {
            return UniqueId();
        }

        UniqueIdData *newId = new UniqueIdData;

        // 1. Extract UUID
        newId->m_uuid = QUuid::fromRfc4122(data.mid(0, 16));

        // 2. Extract CRC16 (Recombine high and low bytes)
        quint8 hi = static_cast<quint8>(data.at(16));
        quint8 lo = static_cast<quint8>(data.at(17));
        newId->m_crc16 = (static_cast<quint16>(hi) << 8) | lo;

        // 3. Extract MD4: 16 bytes starting at 18
        newId->m_md4Hash = data.mid(18, 16);

        // 4. Extract MD5: 16 bytes starting at 34
        newId->m_md5Hash = data.mid(34, 16);

        // 5. Keep the full blob synchronized
        newId->m_uniqueId = data;

        return UniqueId(newId);
    }

    QDataStream &operator<<(QDataStream &outDataStream, const UniqueId & uniqueId)
    {
        uniqueId.insertToDataStream(outDataStream);

        return (outDataStream);
    }

    QDataStream &operator>>(QDataStream &inDataStream, UniqueId & uniqueId)
    {
        uniqueId.extractFromDataStream(inDataStream);

        return (inDataStream);
    }

    UniqueId::UniqueId(UniqueIdData *d) :
        sharedDataP(d)
    {
        // TODO
    }

    UniqueId_Error_t UniqueId::extractFromDataStream(QDataStream & dataStream)
    {
        if (dataStream.version() < UniqueId::dataStreamVersion)
        {
            return (UniqueId_Error_t::UniqueId_Error_Bad_DataStream_Too_Old);
        }

        quint64 dsStreamHeader;
        quint16 dsClassVersion;

        dataStream >> dsStreamHeader >> dsClassVersion;

        if (dsStreamHeader < UniqueId::streamHeader)
        {
            return (UniqueId_Error_t::UniqueId_Error_Bad_StreamHeader_Too_Old);
        }

        if (dsClassVersion < UniqueId::classVersion)
        {
            return (UniqueId_Error_t::UniqueId_Error_Bad_Class_Version_Too_Old);
        }

        qint32 dsSerializeCode;
        dataStream >> dsSerializeCode;

        this->detach(); // Ensure we have our own data before writing to it
        dataStream >> this->sharedDataP->m_uuid;
        dataStream >> this->sharedDataP->m_uniqueId;
        dataStream >> this->sharedDataP->m_md4Hash;
        dataStream >> this->sharedDataP->m_md5Hash;
        dataStream >> this->sharedDataP->m_crc16;

        dataStream >> dsSerializeCode;

        return UniqueId_Error_t::UniqueId_Success;
    }

    UniqueId_Error_t UniqueId::insertToDataStream(QDataStream & dataStream) const
    {
        dataStream.setVersion(UniqueId::dataStreamVersion);

        dataStream << UniqueId::streamHeader << UniqueId::classVersion;

        dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_UNIQUEID_DATA);

        dataStream << this->sharedDataP->m_uuid;
        dataStream << this->sharedDataP->m_uniqueId;
        dataStream << this->sharedDataP->m_md4Hash;
        dataStream << this->sharedDataP->m_md5Hash;
        dataStream << static_cast<quint16>(this->sharedDataP->m_crc16);

        dataStream << static_cast<qint32>(Serialize_t::SERIALIZE_UNIQUEID_DATA_END);

        return UniqueId_Error_t::UniqueId_Success;
    }

} // utils

using utils::UniqueId;

static int registerUniqueIdMetatype()
{
    // Basic registration
    qRegisterMetaType<UniqueId>("UniqueId");

    // If you implemented stream operators for persistence
    qRegisterMetaTypeStreamOperators<UniqueId>("UniqueId");
    return 0;
}

static int dummyUniqueId = registerUniqueIdMetatype();
